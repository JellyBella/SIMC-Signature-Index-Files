// reln.c ... functions on Relations
// part of SIMC signature files
// Written by John Shepherd, September 2018

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "defs.h"
#include "reln.h"
#include "page.h"
#include "tuple.h"
#include "tsig.h"
#include "psig.h"
#include "bits.h"
#include "hash.h"
// open a file with a specified suffix
// - always open for both reading and writing

File openFile(char *name, char *suffix)
{
	char fname[MAXFILENAME];
	sprintf(fname,"%s.%s",name,suffix);
	File f = open(fname,O_RDWR|O_CREAT,0644);
	assert(f >= 0);
	return f;
}

// create a new relation (five files)
// data file has one empty data page

Status newRelation(char *name, Count nattrs, float pF,
                   Count tk, Count tm, Count pm, Count bm)
{
	Reln r = malloc(sizeof(RelnRep));
	RelnParams *p = &(r->params);
	assert(r != NULL);
	p->nattrs = nattrs;
	p->pF = pF,
	p->tupsize = 28 + 7*(nattrs-2);
	Count available = (PAGESIZE-sizeof(Count));
	p->tupPP = available/p->tupsize;
	p->tk = tk; 
	if (tm%8 > 0) tm += 8-(tm%8); // round up to byte size
	p->tm = tm; p->tsigSize = tm/8; p->tsigPP = available/(tm/8);
	if (pm%8 > 0) pm += 8-(pm%8); // round up to byte size
	p->pm = pm; p->psigSize = pm/8; p->psigPP = available/(pm/8);
	if (p->psigPP < 2) { free(r); return -1; }
	if (bm%8 > 0) bm += 8-(bm%8); // round up to byte size
	p->bm = bm; p->bsigSize = bm/8; p->bsigPP = available/(bm/8);
	if (p->bsigPP < 2) { free(r); return -1; }
	r->infof = openFile(name,"info");
	r->dataf = openFile(name,"data");
	r->tsigf = openFile(name,"tsig");
	r->psigf = openFile(name,"psig");
	r->bsigf = openFile(name,"bsig");
	addPage(r->dataf); p->npages = 1; p->ntups = 0;
	addPage(r->tsigf); p->tsigNpages = 1; p->ntsigs = 0;
	addPage(r->psigf); p->psigNpages = 1; p->npsigs = 0;
	addPage(r->bsigf); p->bsigNpages = 1; p->nbsigs = 0; // replace this
	// Create a file containing "pm" all-zeroes bit-strings,
    // each of which has length "bm" bits
	//TODO
	closeRelation(r);
	return 0;
}

// check whether a relation already exists

Bool existsRelation(char *name)
{
	char fname[MAXFILENAME];
	sprintf(fname,"%s.info",name);
	File f = open(fname,O_RDONLY);
	if (f < 0)
		return FALSE;
	else {
		close(f);
		return TRUE;
	}
}

// set up a relation descriptor from relation name
// open files, reads information from rel.info

Reln openRelation(char *name)
{
	Reln r = malloc(sizeof(RelnRep));
	assert(r != NULL);
	r->infof = openFile(name,"info");
	r->dataf = openFile(name,"data");
	r->tsigf = openFile(name,"tsig");
	r->psigf = openFile(name,"psig");
	r->bsigf = openFile(name,"bsig");
	read(r->infof, &(r->params), sizeof(RelnParams));
	return r;
}

// release files and descriptor for an open relation
// copy latest information to .info file
// note: we don't write ChoiceVector since it doesn't change

void closeRelation(Reln r)
{
	// make sure updated global data is put in info file
	lseek(r->infof, 0, SEEK_SET);
	int n = write(r->infof, &(r->params), sizeof(RelnParams));
	assert(n == sizeof(RelnParams));
	close(r->infof); close(r->dataf);
	close(r->tsigf); close(r->psigf); close(r->bsigf);
	free(r);
}

// insert a new tuple into a relation
// returns page where inserted
// returns NO_PAGE if insert fails completely

PageID addToRelation(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL && strlen(t) == tupSize(r));
	Page p;  PageID pid;
	RelnParams *rp = &(r->params);
	
	// add tuple to last page
	//printf("\n--add tuple to last page--\n");
	pid = rp->npages-1;
	p = getPage(r->dataf, pid);
	int newdatapage =0;
	// check if room on last page; if not add new page
//printf("\n--pageNitems:%d--tupPP:%d\n",pageNitems(p),rp->tupPP);
	if (pageNitems(p) == rp->tupPP) {
		addPage(r->dataf);
		rp->npages++;
		pid++;
		free(p);
		p = newPage();
		if (p == NULL) return NO_PAGE;
		newdatapage = 1;
	}
	addTupleToPage(r, p, t);
	rp->ntups++;  //written to disk in closeRelation()
	putPage(r->dataf, pid, p);

	// compute tuple signature and add to tsigf
	//printf("\n--compute tuple signature and update/add to tsigf--\n");
	PageID tsid; Page tp;
	tsid = rp->tsigNpages - 1;
	tp = getPage(r->tsigf,tsid);
	if(pageNitems(tp)==rp->tsigPP){
		addPage(r->tsigf);
		rp->tsigNpages++;
		tsid++;
		free(tp);
		tp = newPage();
		if (tp == NULL)return NO_PAGE;
	}

	Bits tsig = makeTupleSig(r,t);
//printf("----reln-----");
//showBits(tsig);
//printf("\npageNitems:%d\n",pageNitems(tp));
	putBits(tp,pageNitems(tp),tsig);//put sig to page
	addOneItem(tp);
/*Bits tsig1 = makeTupleSig(r,t);
getBits(tp, pageNitems(tp), tsig1);
printf("----reln(in page)-----");
showBits(tsig1);*/
	rp->ntsigs++;
	putPage(r->tsigf,tsid,tp);//put page into file
/*Page p1 = getPage(r->tsigf, tsid);//get page from file
Bits tsig1 = makeTupleSig(r,t);
getBits(p1, pageNitems(p1), tsig1);//get sig from page
printf("----reln(in file)-----");
showBits(tsig1);
printf("\npageNitems:%d\n",pageNitems(tp));*/
	
	// compute page signature and update/add to psigf
	//update when tuple added in the same data page
	//add when tuple added to a new data page
	//as for new pspage, even if current ps at the last position of the pspage, we still need to see if it's update(no need for new pspage,just update) or add(new pspage)
	//printf("\n--compute page signature and update/add to psigf--\n");
	PageID psid; Page pp;
	psid = rp->psigNpages - 1;
	pp = getPage(r->psigf,psid);
	int pageitem = pageNitems(pp);
//printf("~~~~psid:%d~~~pageitem: %d\n",psid,pageitem);
	if(newdatapage || !pageitem){//when data added in new data page or first item in the first page, create new pspage
		if(pageNitems(pp)==rp->psigPP){//last item in pspage, create new pspage for adding ps
			//printf("\nadd new page\n");
			addPage(r->psigf);
			rp->psigNpages++;//pspage++
			psid++;//psid is local pspage 
			free(pp);
			pp = newPage();
			if (pp == NULL)return NO_PAGE;
		}
		//printf("\nadd data into (old as default) page\n");
		Bits psig = makeNewPageSig(r,t);
/*printf("\n-------new page sig-------\n");
showBits(psig);printf("\n");*/
		putBits(pp,pageNitems(pp),psig);//put ps to page
		addOneItem(pp);//pp->nitems++
		rp->npsigs++;//#ps++
		putPage(r->psigf,psid,pp);//put page into file
	}
	else{//update ps in current page
		/*new Tuple is inserted into page PID
		Psig = makePageSig(Tuple)
		PPsig = fetch page signature for data page PID from psigFile
		merge Psig and PPsig giving a new PPsig
		update page signature for data page PID in psigFile*/
		//printf("\nupdate data\n");
		Bits psig = makePageSig(r,t);
		putBits(pp,pageNitems(pp)-1,psig);//update file(same position)
		putPage(r->psigf,psid,pp);
	}


	// use page signature to update bit-slices

	//TODO

	return nPages(r)-1;
}

// displays info about open Reln (for debugging)

void relationStats(Reln r)
{
	RelnParams *p = &(r->params);
	printf("Global Info:\n");
	printf("Dynamic:\n");
    printf("  #items:  tuples: %d  tsigs: %d  psigs: %d  bsigs: %d\n",
			p->ntups, p->ntsigs, p->npsigs, p->nbsigs);
    printf("  #pages:  tuples: %d  tsigs: %d  psigs: %d  bsigs: %d\n",
			p->npages, p->tsigNpages, p->psigNpages, p->bsigNpages);
	printf("Static:\n");
    printf("  tups   #attrs: %d  size: %d bytes  max/page: %d\n",
			p->nattrs, p->tupsize, p->tupPP);
	printf("  sigs   bits/attr: %d\n", p->tk);
	printf("  tsigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->tm, p->tsigSize, p->tsigPP);
	printf("  psigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->pm, p->psigSize, p->psigPP);
	printf("  bsigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->bm, p->bsigSize, p->bsigPP);
}
