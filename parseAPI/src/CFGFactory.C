/*
 * See the dyninst/COPYRIGHT file for copyright information.
 * 
 * We provide the Paradyn Tools (below described as "Paradyn")
 * on an AS IS basis, and do not warrant its validity or performance.
 * We reserve the right to update, modify, or discontinue this
 * software at any time.  We shall have no obligation to supply such
 * updates or modifications or any other form of support to you.
 * 
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "LoopAnalyzer.h"
#include <limits>

#include "CFGFactory.h"
#include "CFG.h"
#include <iostream>

#include "ParseData.h"

using namespace std;
using namespace Dyninst;
using namespace Dyninst::ParseAPI;

std::string ParseAPI::format(EdgeTypeEnum e) {
   switch(e) {
      case CALL:
         return "call";
      case COND_TAKEN:
         return "cond_taken";
      case COND_NOT_TAKEN:
         return "cond_not_taken";
      case INDIRECT:
         return "indirect";
      case DIRECT:
         return "direct";
      case FALLTHROUGH:
         return "fallthrough";
      case CATCH:
         return "catch";
      case CALL_FT:
         return "call_ft";
      case RET:
         return "ret";
      case NOEDGE:
         return "noedge";
      default:
         return "<unknown>";
   }
}

/** The default CFG object factory. Alternative
    implementations might alllocate objects that
    override the default CFG interfaces, or might
    use pooled allocators, etc.
 */

Edge::Edge(Block *source, Block *target, EdgeTypeEnum type)
: _source(source),
  index(source->obj()->parse_data()),
  _target_off(target->low()),
  _type(type,false) { 
      
    }

CFGFactory::~CFGFactory()
{
   destroy_all();
}

// ParseAPI call...
Function *
CFGFactory::_mkfunc(Address addr, FuncSource src, string name, 
    CodeObject * obj, CodeRegion * reg, Dyninst::InstructionSource * isrc)
{
   Function * ret = mkfunc(addr,src,name,obj,reg,isrc);

   // forget about initialization of this function descriptor by this thread
   // before making it available to others. the initialization will not race
   // with a later access by another thread.
   // forget(ret, sizeof(*ret));

   funcs_.add(ret);
   ret->_src =  src;
   return ret;
}

// And user-overriden create
Function *
CFGFactory::mkfunc(Address addr, FuncSource, string name, 
    CodeObject * obj, CodeRegion * reg, Dyninst::InstructionSource * isrc)
{
    Function * ret = new Function(addr,name,obj,reg,isrc);

    return ret;
}

Block *
CFGFactory::_mkblock(Function *  f , CodeRegion *r, Address addr)
{
   Block * ret = mkblock(f, r, addr);
   // forget(ret, sizeof(*ret));
   blocks_.add(ret);
   return ret;
}

Block *
CFGFactory::_mkblock(CodeObject* co, CodeRegion *r, Address addr)
{
   Block* ret = new Block(co, r, addr);
   // forget(ret, sizeof(*ret));
   blocks_.add(ret);
   return ret;
}

Block *
CFGFactory::mkblock(Function *  f , CodeRegion *r, Address addr) {

    Block * ret = new Block(f->obj(),r,addr, f);
    return ret;
}


Block *
CFGFactory::_mksink(CodeObject * obj, CodeRegion *r) {
   Block * ret = mksink(obj,r);
   blocks_.add(ret);
   return ret;
}

Block *
CFGFactory::mksink(CodeObject * obj, CodeRegion *r) {
    Block * ret = new Block(obj,r,numeric_limits<Address>::max());
    return ret;
}

Edge *
CFGFactory::_mkedge(Block * src, Block * trg, EdgeTypeEnum type) {
    Edge * ret = mkedge(src,trg,type);
    edges_.add(ret);
    return ret;
}

Edge *
CFGFactory::mkedge(Block * src, Block * trg, EdgeTypeEnum type) {
    Edge * ret = new Edge(src,trg,type);
    return ret;
}

void CFGFactory::destroy_func(Function *f) {
    boost::lock_guard<CFGFactory> g(*this);
   f->remove();
   free_func(f);
}

void
CFGFactory::free_func(Function *f) {
    delete f;
}

void
CFGFactory::destroy_block(Block *b) {
    boost::lock_guard<CFGFactory> g(*this);
    b->remove();
    free_block(b);
}

void
CFGFactory::free_block(Block *b) {
    delete b;
}

std::string to_str(EdgeState e)
{
    switch(e)
    {
        case created: return "ok";
        case destroyed_noreturn: return "destroyed fallthrough from non-returning call/syscall";
        case destroyed_cb: return "destroyed from callback";
        case destroyed_all: return "destroyed during global cleanup";
        default: return "ERROR: unknown state";
    }
}

void
CFGFactory::destroy_edge(Edge *e, Dyninst::ParseAPI::EdgeState reason) {
    boost::lock_guard<CFGFactory> g(*this);
    e->remove();
    if(reason == destroyed_all) {
        free_edge(e);
    }
}

void
CFGFactory::free_edge(Edge *e) {
   delete e;
}

void
CFGFactory::destroy_all() {
    boost::lock_guard<CFGFactory> g(*this);
    // XXX carefully calling free_* routines; could be faster and just
    // call delete

    fact_list<Edge *>::iterator eit = edges_.begin();
    while(eit != edges_.end()) {
        fact_list<Edge *>::iterator cur = eit++;
        destroy_edge(*cur, destroyed_all);
    }
    fact_list<Block *>::iterator bit = blocks_.begin();
    while(bit != blocks_.end()) {
        fact_list<Block *>::iterator cur = bit++;
        destroy_block(*cur);
    }
    fact_list<Function *>::iterator fit = funcs_.begin();
    while(fit != funcs_.end()) {
        fact_list<Function *>::iterator cur = fit++;
        destroy_func(*cur);
    }
}

