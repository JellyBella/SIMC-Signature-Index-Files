# SIMC-Signature-Index-Files

The goal is to build a simple implementation of a SIMC signature file, including application to create SIMC files, insert tuples into them, and search for tuples based on partial-match retrieval queries.

bits.c file produces a working bit-string data type.
The startQuery() function parses the query string and then uses the appropriate type of signature to generate a list of pages which potentially contain matching tuples. This list is implemented as a bit-string where a 1 indicates a page which needs to be checked for matches. At this stage, all of the signature types mark all pages as potential matches, so all pages need to be checked.

The addToRelation() function inserts a tuple into the next available slot in the data file, but currently does nothing with signatures. You should add code here which generates a tuple signature for the new tuple and inserts it in the next available slot in the Rel.tsig file.

The makeTupleSig() function takes a tuple and returns a bit-string which contains a superimposed codeword signature for that tuple. 

The addToRelation() function is to maintain page signatures when new tuples are inserted. One major difference between tuple signatures and page signatures is that page signatures are not a one-off insertion. When a new tuple is added, its page-level signature needs to be included page signature for the page where it it is inserted. The makePageSig() function be used to generate a page-level signature for the query, and then used to generate a bit-string of matching pages.

Implement indexing using bit-sliced page signatures.

Each bit-slice is effectively a list of pages that have a specific bit from the page-signature set to 1 (e.g. if a page-level signature has bit 5 set to one, the bit-slice 5 has a 1 bit for every page with a page signature where bit 5 is set). This drives both the updating of bit-slices and their use in indexing.

The addToRelation() should take a tuple, produce a page signature for it, then update all of the bit-slices corresponding to 1-bits in the page signature. The findPagesUsingBitSlices() function computes a page-level signature for query and then takes an intersection of the bit-slices corresponding to the 1-bits in the page signature. This gives a "matching" pages list straight away, and hopefully after reading far less of the Rel.bsig file than would be read using a Rel.psig file.