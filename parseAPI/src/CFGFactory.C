/*
 * Copyright (c) 1996-2009 Barton P. Miller
 * 
 * We provide the Paradyn Parallel Performance Tools (below
 * described as "Paradyn") on an AS IS basis, and do not warrant its
 * validity or performance.  We reserve the right to update, modify,
 * or discontinue this software at any time.  We shall have no
 * obligation to supply such updates or modifications or any other
 * form of support to you.
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
#include <limits>

#include "CFGFactory.h"
#include "CFG.h"

using namespace Dyninst;
using namespace Dyninst::ParseAPI;

/** The default CFG object factory. Alternative
    implementations might alllocate objects that
    override the default CFG interfaces, or might
    use pooled allocators, etc.
 */

CFGFactory::~CFGFactory()
{
    free_all();
}

Function *
CFGFactory::mkfunc(Address addr, FuncSource src, string name, 
    CodeObject * obj, CodeRegion * reg, Dyninst::InstructionSource * isrc)
{
    Function * ret = new Function(addr,name,obj,reg,isrc);
    funcs_.add(*ret);
    ret->_src =  src;
    return ret;
}

void
CFGFactory::free_func(Function *f) {
    f->remove();
    delete f;
}

Block *
CFGFactory::mkblock(Function *  f , CodeRegion *r, Address addr) {
    Block * ret = new Block(f->obj(),r,addr);
    blocks_.add(*ret);
    return ret;
}
Block *
CFGFactory::mksink(CodeObject * obj, CodeRegion *r) {
    Block * ret = new Block(obj,r,numeric_limits<Address>::max());
    blocks_.add(*ret);
    return ret;
}
void
CFGFactory::free_block(Block *b) {
    b->remove();
    delete b;
}

Edge *
CFGFactory::mkedge(Block * src, Block * trg, EdgeTypeEnum type) {
    Edge * ret = new Edge(src,trg,type);
    edges_.add(*ret);
    return ret;
}
void
CFGFactory::free_edge(Edge *e) {
    e->remove();
    delete e;
}

void
CFGFactory::free_all() {
    // XXX carefully calling free_* routines; could be faster and just
    // call delete
    fact_list<Edge>::iterator eit = edges_.begin();
    while(eit != edges_.end()) {
        fact_list<Edge>::iterator cur = eit++;
        free_edge(&*cur);
    }
    fact_list<Block>::iterator bit = blocks_.begin();
    while(bit != blocks_.end()) {
        fact_list<Block>::iterator cur = bit++;
        free_block(&*cur);
    }
    fact_list<Function>::iterator fit = funcs_.begin();
    while(fit != funcs_.end()) {
        fact_list<Function>::iterator cur = fit++;
        free_func(&*cur);
    }
}