// psig.c ... functions on page signatures (psig's)
// part of SIMC signature files
// Written by John Shepherd, September 2018

#include "defs.h"
#include "reln.h"
#include "query.h"
#include "psig.h"
#include "tsig.h"

Bits makeNewPageSig(Reln r, Tuple t)
{
	RelnParams *rp = &(r->params);
	char** attr_value = tupleVals(r,t);
	assert(r != NULL && t != NULL);
	Bits desc = newBits(rp->pm);
	Bits cword = newBits(rp->pm);
	//printf("%s\n","hello.dajhsgdjyasgdjha.");	
	int j; 
//printf("\n-------cword-------\n");
	for (j = 0; j < rp->nattrs; j++) {
		if(attr_value[j][0]!='?')cword = codeword(attr_value[j],rp->pm,rp->tk);
		else continue;
//showBits(cword);printf("\n");
		orBits(desc,cword);
	}
//printf("\n----final desc-----\n");
//showBits(desc);printf("\n");
	return desc;
}

Bits makePageSig(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL);
	RelnParams *rp = &(r->params);
	PageID psid; Page pp;
	psid = rp->psigNpages - 1;
	pp = getPage(r->psigf,psid);
	Bits psig = newBits(rp->pm);
//printf("\n!!!pageNitems: %d",pageNitems(pp));
	getBits(pp,pageNitems(pp)-1,psig);//get ps from file
/*printf("\n--------------old page sig---------------------\n");
showBits(psig);*/
	Bits desc = makeNewPageSig(r,t);
/*printf("\n-------current page sig-------\n");
showBits(desc);	*/
	orBits(desc,psig);
	//printf("pageNitems:%d\n",pageNitems(pp));
/*printf("\n--------------updated page sig---------------------\n");
showBits(desc);printf("\n");*/
	return desc;
}

void findPagesUsingPageSigs(Query q)
{
	assert(q != NULL);	
	RelnParams *rp = &(q->rel->params);
	Bits querySig = makeNewPageSig(q->rel,q->qstring);
/*printf("\n-----Query Page Sig----\n");
showBits(querySig);*/
	//setAllBits(q->pages);
	int i,j;
//printf(" nsigNpages:%d\n",rp->tsigNpages);
	unsetAllBits(q->pages);
	for(i=0;i<rp->psigNpages;i++){
		Page p = getPage(q->rel->psigf,i);
		int npsig = pageNitems(p);
//printf("page:%d, nitems:%d\n",i,ntsig);
		for(j=0;j<npsig;j++){
			Bits bitstring = newBits(rp->pm);
			unsetAllBits(bitstring);
			getBits(p,j,bitstring);//get bits from pspage to bitstring
//printf("\n----findPagesUsingTupSigs----\n");
//showBits(bitstring);
			if(isSubset(querySig,bitstring)){setBit(q->pages,i); /*printf("--item#:%d\n",j);showBits(q->pages);*/}
		}
	}
	// The printf below is primarily for debugging
	// Remove it before submitting this function
	//printf("Matched Pages:"); showBits(q->pages); putchar('\n');
}

