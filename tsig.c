// tsig.c ... functions on Tuple Signatures (tsig's)
// part of SIMC signature files
// Written by John Shepherd, September 2018

#include <unistd.h>
#include <string.h>
#include "defs.h"
#include "tsig.h"
#include "reln.h"
#include "hash.h"
#include "bits.h"

// make a tuple signature

Bits codeword(char *attr_value, int m, int k)
{
   int  nbits = 0;   // count of set bits
   Bits cword = newBits(m);
   unsetAllBits(cword);
//showBits(cword);
   srandom(hash_any(attr_value,strlen(attr_value)));
   while (nbits < k) {
      int i = random() % m;
	//printf("%d\n",i);
      if (!bitIsSet(cword,i)) {
         setBit(cword,i);
         nbits++;
//printf("--in loop--");
      }
//printf(" m:%d,k:%d\n",m,k);
   }
//printf("\n----cword-----\n");
//showBits(cword);
   return cword;  // m-bits with k 1-bits and m-k 0-bits
}

Bits makeTupleSig(Reln r, Tuple t)
{
	RelnParams *rp = &(r->params);
	char** attr_value = tupleVals(r,t);
	assert(r != NULL && t != NULL);
	Bits desc = newBits(rp->tm);
	Bits cword = newBits(rp->tm);
	//printf("%s\n","hello.dajhsgdjyasgdjha.");	
	int j; 
//printf("\n-------cword-------\n");
	for (j = 0; j < rp->nattrs; j++) {
		if(attr_value[j][0]!='?')cword = codeword(attr_value[j],rp->tm,rp->tk);
		else continue;
//showBits(cword);printf("\n");
		orBits(desc,cword);
	}
//printf("\n----final desc-----\n");
//showBits(desc);printf("\n");
	return desc;
}

// find "matching" pages using tuple signatures

void findPagesUsingTupSigs(Query q)
{
	assert(q != NULL);	
	RelnParams *rp = &(q->rel->params);
	Bits querySig = makeTupleSig(q->rel, q->qstring);
//printf("\n-----querySig----\n");
//showBits(querySig);
	setAllBits(q->pages);
	int i,j;
//printf(" nsigNpages:%d\n",rp->tsigNpages);
	unsetAllBits(q->pages);
	for(i=0;i<rp->tsigNpages;i++){
		Page p = getPage(q->rel->tsigf,i);
		int ntsig = pageNitems(p);
//printf("page:%d, nitems:%d\n",i,ntsig);
		for(j=0;j<ntsig;j++){
			Bits bitstring = newBits(rp->tm);
			unsetAllBits(bitstring);
			getBits(p,j,bitstring);//get bits from page to bitstring
//printf("\n----findPagesUsingTupSigs----\n");
//showBits(bitstring);
			if(isSubset(querySig,bitstring)){setBit(q->pages,i); /*printf("--item#:%d\n",j);showBits(q->pages);*/}
		}
	}
	// The printf below is primarily for debugging
	// Remove it before submitting this function
	//printf("Matched Pages:"); showBits(q->pages); putchar('\n');
}
