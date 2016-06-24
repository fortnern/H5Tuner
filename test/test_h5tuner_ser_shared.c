/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Example of using the parallel HDF5 library to access datasets.
 * Last revised: April 24, 2001.
 *
 * This program contains two parts.  In the first part, the mpi processes
 * collectively create a new parallel HDF5 file and create two fixed
 * dimension datasets in it.  Then each process writes a hyperslab into
 * each dataset in an independent mode.  All processes collectively
 * close the datasets and the file.
 * In the second part, the processes collectively open the created file
 * and the two datasets in it.  Then each process reads a hyperslab from
 * each dataset in an independent mode and prints them out.
 * All processes collectively close the datasets and the file.
 *
 * The need of requirement of parallel file prefix is that in general
 * the current working directory in which compiling is done, is not suitable
 * for parallel I/O and there is no standard pathname for parallel file
 * systems.  In some cases, the parallel file name may even needs some
 * parallel file type prefix such as: "pfs:/GF/...".  Therefore, this
 * example requires an explicite parallel file prefix.  See the usage
 * for more detail.
 */

#include <assert.h>
#include "hdf5.h"
#include <string.h>
#include <stdlib.h>

/* Temporary source code */
#define FAIL -1
/* temporary code end */

/* Define some handy debugging shorthands, routines, ... */
/* debugging tools */
#define MESG(x)\
    if (verbose) printf("%s\n", x);

/* End of Define some handy debugging shorthands, routines, ... */

/* Constants definitions */
/* 24 is a multiple of 2, 3, 4, 6, 8, 12.  Neat for parallel tests. */
#define SPACE1_DIM1     24
#define SPACE1_DIM2     24
#define SPACE1_RANK     2
#define DATASETNAME1    "Data1"
#define DATASETNAME2    "Data2"
#define DATASETNAME3    "Data3"
/* hyperslab layout styles */
#define BYROW           1   /* divide into slabs of rows */
#define BYCOL           2   /* divide into blocks of columns */


/* dataset data type.  Int's can be easily octo dumped. */
typedef int DATATYPE;

/* global variables */
int nerrors = 0;                                /* errors count */
#ifndef PATH_MAX
#define PATH_MAX    512
#endif  /* !PATH_MAX */
char    testfiles[3][PATH_MAX];


/* option flags */
int verbose = 0;                        /* verbose, default as no. */
int doread=1;                           /* read test */
int dowrite=1;                          /* write test */
int docleanup=1;                        /* cleanup */

/* Prototypes */
void dataset_fill(hsize_t dims[], DATATYPE * dataset);
void dataset_print(hsize_t dims[], DATATYPE * dataset);
int dataset_vrfy(hsize_t dims[], DATATYPE *dataset, DATATYPE *original);
void hdf5writeAll(char *filename);
void hdf5readAll(char *filename);
int parse_options(int argc, char **argv);
void usage(void);
int mkfilenames();
void cleanup(void);



/*
 * Fill the dataset with trivial data for testing.
 * Assume dimension rank is 2 and data is stored contiguous.
 */
void
dataset_fill(hsize_t dims[], DATATYPE *dataset)
{
    DATATYPE *dataptr = dataset;
    hsize_t i, j;

    /* put some trivial data in the data_array */
    for (i=0; i < dims[0]; i++){
        for (j=0; j < dims[1]; j++){
            *dataptr++ = i*100 + j;
        }
    }
}


/*
 * Print the content of the dataset.
 */
void dataset_print(hsize_t dims[], DATATYPE * dataset)
{
    DATATYPE *dataptr = dataset;
    hsize_t i, j;

    /* print the slab read */
    for (i=0; i < dims[0]; i++){
        printf("Row %lu: ", (unsigned long)i);
        for (j=0; j < dims[1]; j++){
            printf("%03d ", *dataptr++);
        }
        printf("\n");
    }
}


/*
 * Print the content of the dataset.
 */
int dataset_vrfy(hsize_t dims[], DATATYPE *dataset, DATATYPE *original)
{
#define MAX_ERR_REPORT	10		/* Maximum number of errors reported */

    hsize_t i, j;
    int nerr;

    /* print it if verbose */
    if (verbose)
        dataset_print(dims, dataset);

    nerr = 0;
    for (i=0; i < dims[0]; i++){
        for (j=0; j < dims[1]; j++){
            if (*dataset++ != *original++){
                nerr++;
                if (nerr <= MAX_ERR_REPORT){
                    printf("Dataset Verify failed at [%lu][%lu]: expect %d, got %d\n",
                        (unsigned long)i, (unsigned long)j,
                        *(dataset-1), *(original-1));
                }
            }
        }
    }
    if (nerr > MAX_ERR_REPORT)
        printf("[more errors ...]\n");
    if (nerr)
        printf("%d errors found in dataset_vrfy\n", nerr);
    return(nerr);
}


/* Function to check if a dataset's DCPL has been set up correctly by H5Tuner */
void
test_dcpl(hid_t dset_id, const char *dset_name, const char *base_filename)
{
    hid_t dcpl_id;
    H5D_layout_t layout;
    hsize_t cdims[SPACE1_RANK];     /* Chunk dimensions */
    int ndims;
    herr_t ret = 0;                 /* Generic return value */

    if ( verbose ) {
        printf("\n\n--------------------------------------------------\n");
        printf("Testing chunk dimensions\n");
        printf("--------------------------------------------------\n");
    }

    /* Retrieve dataset's DCPL */
    dcpl_id = H5Dget_create_plist(dset_id);
    assert(dcpl_id != FAIL);

    /* Check layout of dcpl_id */
    layout = H5Pget_layout(dcpl_id);
    assert(layout != H5D_LAYOUT_ERROR);
    if(layout == H5D_CHUNKED) {
        if(verbose)
            printf("PASSED: Retrieved layout type\n");
    }
    else {
        ret = FAIL;
        nerrors++;
        printf("FAILED: Retrieved layout type\n");
    }

    /* Get chunk dimensions */
    ndims = H5Pget_chunk(dcpl_id, SPACE1_RANK, cdims);
    assert(ndims != FAIL);
    if(ndims != SPACE1_RANK) {
        ret = FAIL;
        nerrors++;
        printf("FAILED: Retrieved layout chunk rank\n");
    }

    /* Check chunk dimensions */
    if(!strcmp(base_filename, "ParaEg2.h5")) {
        if(cdims[0] == 4) {
            if(verbose)
                printf("PASSED: cdims[0]\n");
        }
        else {
            ret = FAIL;
            nerrors++;
            printf("FAILED: Retrieved layout chunk dims[0]\n");
            printf("Test value set to: 4\nRetrieved cdims[0]=%llu\n", (long long unsigned)cdims[0]);
        }
    }
    else {
        if(cdims[0] == 6) {
            if(verbose)
                printf("PASSED: cdims[0]\n");
        }
        else {
            ret = FAIL;
            nerrors++;
            printf("FAILED: Retrieved layout chunk dims[0]\n");
            printf("Test value set to: 6\nRetrieved cdims[0]=%llu\n", (long long unsigned)cdims[0]);
        }
    }
    if(!strcmp(dset_name, "Data2")) {
        if(cdims[1] == 7) {
            if(verbose)
                printf("PASSED: cdims[1]\n");
        }
        else {
            ret = FAIL;
            nerrors++;
            printf("FAILED: Retrieved layout chunk dims[1]\n");
            printf("Test value set to: 7\nRetrieved cdims[1]=%llu\n", (long long unsigned)cdims[1]);
        }
    }
    else {
        if(cdims[1] == 5) {
            if(verbose)
                printf("PASSED: cdims[1]\n");
        }
        else {
            ret = FAIL;
            nerrors++;
            printf("FAILED: Retrieved layout chunk dims[1]\n");
            printf("Test value set to: 5\nRetrieved cdims[1]=%llu\n", (long long unsigned)cdims[1]);
        }
    }
    assert(ret != FAIL);

    ret = H5Pclose(dcpl_id);
    assert(ret != FAIL);

    return;
}


/*
 * Example of using the HDF5 library to create two datasets
 */

void
hdf5writeAll(char *filename)
{
    hid_t fid1;                 /* HDF5 file IDs */
    hid_t acc_tpl1;             /* File access templates */
    hid_t acc_tpl2;             /* FAPL retrieved from file */
    hid_t dcpl_id;              /* Default DCPL */
    hid_t sid1;                 /* Dataspace ID */
    hid_t dataset1, dataset2;   /* Dataset ID */
    hsize_t dims1[SPACE1_RANK] =
        {SPACE1_DIM1,SPACE1_DIM2}; /* dataspace dim sizes */
    DATATYPE data_array1[SPACE1_DIM1][SPACE1_DIM2]; /* data buffer */

    /* in support of H5Tuner Test */
    char *libtuner_file = getenv("LD_PRELOAD");
    hsize_t alignment[2];
    size_t sieve_buf_size;
    H5D_layout_t layout;

    const char *base_filename;

    herr_t tmp_ret;
    herr_t ret;                 /* Generic return value */

    /* Retrieve base name for filename */
    if(NULL == (base_filename = strrchr(filename, '/')))
        base_filename = filename;
    else
        base_filename++;

    /* in support of H5Tuner Test */

    if (verbose)
        printf("Write test on file %s\n", filename);

    /* -------------------
     * START AN HDF5 FILE
     * -------------------*/
    /* setup file access template */
    acc_tpl1 = H5Pcreate (H5P_FILE_ACCESS);
    assert(acc_tpl1 != FAIL);
    MESG("H5Pcreate access succeed");

    /* create the file collectively */
    fid1=H5Fcreate(filename,H5F_ACC_TRUNC,H5P_DEFAULT,acc_tpl1);
    assert(fid1 != FAIL);
    MESG("H5Fcreate succeed");

    /* ------------------------------------------------
       H5Tuner tests
       ------------------------------------------------ */

    /* Retrieve  parameters set via the H5Tuner */
    printf("\n\n--------------------------------------------------\n");
    if ( (libtuner_file != NULL) && (strlen(libtuner_file) > 1) ){
        printf("Version of the H5Tuner loaded: \n%s\n", libtuner_file);
    }
    else {
        printf("No H5Tuner currently loaded.\n");
    }
    printf("--------------------------------------------------\n");

    /* Retrieve FAPL from file */
    acc_tpl2 = H5Fget_access_plist(fid1);
    assert(acc_tpl1 != FAIL);

    /* Retrieve default HDF5 Threshold and Alignment */
    ret = H5Pget_alignment(acc_tpl1, &alignment[0], &alignment[1]);
    assert(ret != FAIL);

    /* Verify default threshold and alignment */
    if(alignment[0] != 1) {
        ret = FAIL;
        nerrors++;
        printf("FAILED: Default Threshold Test\n");
    }
    if(alignment[1] != 1) {
        ret = FAIL;
        nerrors++;
        printf("FAILED: Default Alignment Test\n");
    }

    /* Retrieve file HDF5 Threshold and Alignment */
    ret = H5Pget_alignment(acc_tpl2, &alignment[0], &alignment[1]);
    assert(ret != FAIL);

    if ( verbose ) {
        MESG("H5Pget_alignment succeed. Values Retrieved");
        printf("\n\n--------------------------------------------------\n");
        printf("Testing values for Threshold\n");
        printf("--------------------------------------------------\n");
        printf("Test value set to:88 \nRetrieved Threshold=%lu\n", alignment[0]);
    }
    /* Check Threshold */
    if ( alignment[0] == 88 ) {
        if (verbose)
            printf("PASSED: Threshold Test\n");
    }
    else {
        ret = FAIL;
        nerrors++;
        printf("FAILED: Threshold Test\n");
    }
    assert(ret != FAIL);
    if ( verbose ) {
        printf("\n\n--------------------------------------------------\n");
        printf("Testing values for Alignment\n");
        printf("--------------------------------------------------\n");
        printf("Test value set to:44 \nRetrieved Alignment=%lu\n", alignment[1]);
    }
    /* Check Alignment */
    if ( alignment[1] == 44 ) {
        if (verbose)
            printf("PASSED: Alignment Test\n");
    }
    else {
        ret = FAIL;
        nerrors++;
        printf("FAILED: Alignment Test\n");
    }
    assert(ret != FAIL);

    /* Retrieve default sieve buffer size */
    ret = H5Pget_sieve_buf_size(acc_tpl1, &sieve_buf_size);
    assert(ret != FAIL);

    /* Verify default sieve buffer size */
    if(sieve_buf_size != 65536) {
        ret = FAIL;
        nerrors++;
        printf("FAILED: Default Sieve Buffer Size Test\n");
    }

    /* Retrieve file sieve buffer size */
    ret = H5Pget_sieve_buf_size(acc_tpl2, &sieve_buf_size);
    assert(ret != FAIL);
    MESG("H5Pget_sieve_buf_size succeed. Value Retrieved");
    if ( verbose ) {
        printf("\n\n--------------------------------------------------\n");
        printf("Testing values for Sieve Buffer Size\n");
        printf("--------------------------------------------------\n");
        printf("Test value set to:77 \nRetrieved Sieve Buffer Size=%lu\n", sieve_buf_size);
    }

    /* Check sieve buffer size */
    if ( (int) sieve_buf_size == 77 ) {
        if ( verbose )
            printf("PASSED: Sieve Buffer Size Test\n");
    }
    else {
        ret = FAIL;
        nerrors++;
        printf("FAILED: Sieve Buffer Size Test\n");
    }
    assert(ret != FAIL);

    /* end of H5Tuner tests
       --------------------------------------- */


    /* Release file-access templates */
    ret=H5Pclose(acc_tpl1);
    assert(ret != FAIL);
    ret=H5Pclose(acc_tpl2);
    assert(ret != FAIL);


    /* --------------------------
     * Define the dimensions of the overall datasets
     * and create the dataset
     * ------------------------- */
    /* setup dimensionality object */
    sid1 = H5Screate_simple (SPACE1_RANK, dims1, NULL);
    assert (sid1 != FAIL);
    MESG("H5Screate_simple succeed");

    /* Set up default DCPL */
    dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
    assert(dcpl_id != FAIL);

    /* Check layout of dcpl_id */
    layout = H5Pget_layout(dcpl_id);
    assert(layout != H5D_LAYOUT_ERROR);
    if(layout != H5D_CONTIGUOUS) {
        ret = FAIL;
        nerrors++;
        printf("FAILED: Default layout type\n");
    }
    assert(ret != FAIL);

    /* create a dataset collectively */
    dataset1 = H5Dcreate2(fid1, DATASETNAME1, H5T_NATIVE_INT, sid1, H5P_DEFAULT, dcpl_id, H5P_DEFAULT);
    assert(dataset1 != FAIL);
    MESG("H5Dcreate2 succeed");

    /* create another dataset collectively */
    dataset2 = H5Dcreate2(fid1, DATASETNAME2, H5T_NATIVE_INT, sid1, H5P_DEFAULT, dcpl_id, H5P_DEFAULT);
    assert(dataset2 != FAIL);
    MESG("H5Dcreate2 2 succeed");

    /* ------------------------------------------------
       H5Tuner tests
       ------------------------------------------------ */

    /* Check layout of dcpl_id (should not have changed) */
    layout = H5Pget_layout(dcpl_id);
    assert(layout != H5D_LAYOUT_ERROR);
    if(layout != H5D_CONTIGUOUS) {
        ret = FAIL;
        nerrors++;
        printf("FAILED: Default layout type post H5Dcreate\n");
    }
    assert(ret != FAIL);

    test_dcpl(dataset1, DATASETNAME1, base_filename);
    test_dcpl(dataset2, DATASETNAME2, base_filename);

    /* end of H5Tuner tests
       --------------------------------------- */

    /* Close dcpl_id */
    ret = H5Pclose(dcpl_id);
    assert(ret != FAIL);

    /* fill the local slab with some trivial data */
    dataset_fill(dims1, &data_array1[0][0]);
    MESG("data_array initialized");
    if (verbose){
        MESG("data_array created");
        dataset_print(dims1, &data_array1[0][0]);
    }

    /* write data collectively */
    ret = H5Dwrite(dataset1, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_array1);
    assert(ret != FAIL);
    MESG("H5Dwrite succeed");

    /* put some trivial data in the data_array */
    dataset_fill(dims1, &data_array1[0][0]);
    MESG("data_array initialized");
    if (verbose){
        MESG("data_array created");
        dataset_print(dims1, &data_array1[0][0]);
    }

    /* write data independently */
    ret = H5Dwrite(dataset2, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_array1);
    assert(ret != FAIL);
    MESG("H5Dwrite succeed");


    /*
     * All writes completed.  Close datasets collectively
     */
    ret=H5Dclose(dataset1);
    assert(ret != FAIL);
    MESG("H5Dclose1 succeed");
    ret=H5Dclose(dataset2);
    assert(ret != FAIL);
    MESG("H5Dclose2 succeed");

    /* release all IDs created */
    H5Sclose(sid1);

    /* close the file collectively */
    H5Fclose(fid1);
}

/*
 * Example of using the parallel HDF5 library to read two datasets
 * in one HDF5 file with collective parallel access support.
 * The Datasets are of sizes (number-of-mpi-processes x DIM1) x DIM2.
 * Each process controls only a slab of size DIM1 x DIM2 within each
 * dataset. [Note: not so yet.  Datasets are of sizes DIM1xDIM2 and
 * each process controls a hyperslab within.]
 */

void
hdf5readAll(char *filename)
{
    hid_t fid1;                 /* HDF5 file IDs */
    hid_t acc_tpl1;             /* File access templates */
    hid_t dcpl_id;              /* Default DCPL */
    hid_t dataset1, dataset2;   /* Dataset ID */
    hsize_t dims1[SPACE1_RANK] =
        {SPACE1_DIM1,SPACE1_DIM2}; /* dataspace dim sizes */
    DATATYPE data_array1[SPACE1_DIM1][SPACE1_DIM2]; /* data buffer */
    DATATYPE data_origin1[SPACE1_DIM1][SPACE1_DIM2]; /* expected data buffer */

    const char *base_filename;

    herr_t ret;                 /* Generic return value */

    /* Retrieve base name for filename */
    if(NULL == (base_filename = strrchr(filename, '/')))
        base_filename = filename;
    else
        base_filename++;

    if (verbose)
        printf("Read test on file %s\n", filename);

    /* -------------------
     * OPEN AN HDF5 FILE
     * -------------------*/
    /* setup file access template. */
    acc_tpl1 = H5Pcreate (H5P_FILE_ACCESS);
    assert(acc_tpl1 != FAIL);
    MESG("H5Pcreate access succeed");

    /* open the file */
    fid1=H5Fopen(filename,H5F_ACC_RDWR,acc_tpl1);
    assert(fid1 != FAIL);
    MESG("H5Fopen succeed");

    /* Release file-access template */
    ret=H5Pclose(acc_tpl1);
    assert(ret != FAIL);


    /* --------------------------
     * Open the datasets in it
     * ------------------------- */
    /* open the dataset1 */
    dataset1 = H5Dopen2(fid1, DATASETNAME1, H5P_DEFAULT);
    assert(dataset1 != FAIL);
    MESG("H5Dopen2 succeed");

    /* open another dataset */
    dataset2 = H5Dopen2(fid1, DATASETNAME2, H5P_DEFAULT);
    assert(dataset2 != FAIL);
    MESG("H5Dopen2 2 succeed");

    /* ------------------------------------------------
       H5Tuner tests
       ------------------------------------------------ */

    test_dcpl(dataset1, DATASETNAME1, base_filename);
    test_dcpl(dataset2, DATASETNAME2, base_filename);

    /* end of H5Tuner tests
       --------------------------------------- */

    /*
     * Set up dimensions of the slab this process accesses.
     */

    /* fill dataset with test data */
    dataset_fill(dims1, &data_origin1[0][0]);
    MESG("data_array initialized");
    if (verbose){
        MESG("data_array created");
        dataset_print(dims1, &data_array1[0][0]);
    }

    /* read data */
    ret = H5Dread(dataset1, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_array1);
    assert(ret != FAIL);
    MESG("H5Dread succeed");

    /* verify the read data with original expected data */
    ret = dataset_vrfy(dims1, &data_array1[0][0], &data_origin1[0][0]);
    assert(ret != FAIL);

    /* fill dataset with test data */
    dataset_fill(dims1, &data_origin1[0][0]);
    MESG("data_array initialized");
    if (verbose){
        MESG("data_array created");
        dataset_print(dims1, &data_array1[0][0]);
    }

    /* read data */
    ret = H5Dread(dataset2, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data_array1);
    assert(ret != FAIL);
    MESG("H5Dread succeed");

    /* verify the read data with original expected data */
    ret = dataset_vrfy(dims1, &data_array1[0][0], &data_origin1[0][0]);
    assert(ret != FAIL);


    /*
     * All reads completed.  Close datasets
     */
    ret=H5Dclose(dataset1);
    assert(ret != FAIL);
    MESG("H5Dclose1 succeed");
    ret=H5Dclose(dataset2);
    assert(ret != FAIL);
    MESG("H5Dclose2 succeed");

    /* close the file collectively */
    H5Fclose(fid1);
}


/*
 * Show command usage
 */
void
usage(void)
{
    printf("Usage: testphdf5 [-f <prefix>] [-r] [-w] [-v]\n");
    printf("\t-c\tno cleanup\n");
    printf("\t-r\tno read\n");
    printf("\t-w\tno write\n");
    printf("\t-v\tverbose on\n");
    printf("\tdefault do write then read\n");
    printf("\n");
}


/*
 * compose the test filename with the prefix supplied.
 * return code: 0 if no error
 *              1 otherwise.
 */
int
mkfilenames()
{
    int i, n;
    size_t strsize = 12;

    /* filename will be prefix/ParaEgN.h5 where N is 0 to 9. */
    /* So, string must be big enough to hold the prefix, / and 10 more chars */
    /* and the terminating null. */
    if (strsize > PATH_MAX){
        printf("File prefix too long;  Use a short path name.\n");
        return(1);
    }
    n = sizeof(testfiles)/sizeof(testfiles[0]);
    if (n > 9){
        printf("Warning: Too many entries in testfiles. "
            "Need to adjust the code to accommodate the large size.\n");
    }
    for (i=0; i<n; i++){
        sprintf(testfiles[i], "./ParaEg%d.h5", i);
    }
    return(0);

}


/*
 * parse the command line options
 */
int
parse_options(int argc, char **argv){
    int i, n;

    /* initialize testfiles to nulls */
    n = sizeof(testfiles)/sizeof(testfiles[0]);
    for (i=0; i<n; i++){
        testfiles[i][0] = '\0';
    }

    while (--argc){
        if (**(++argv) != '-'){
            break;
        }else{
            switch(*(*argv+1)){
                case 'c':   docleanup = 0;	/* no cleanup */
                    break;
                case 'r':   doread = 0;
                    break;
                case 'w':   dowrite = 0;
                    break;
                case 'v':   verbose = 1;
                    break;
                default:    usage();
                    nerrors++;
                    return(1);
            }
        }
    }

    /* check the file prefix */
    if (testfiles[0][0] == '\0'){
        mkfilenames();
    }
    return(0);
}


/*
 * cleanup test files created
 */
void
cleanup(void)
{
    int i, n;

    n = sizeof(testfiles)/sizeof(testfiles[0]);
    for (i=0; i<n; i++){
        remove(testfiles[i]);
    }
}


/* Main Program */
int
main(int argc, char **argv)
{
    int i, n;

    if (parse_options(argc, argv) != 0)
        goto finish;

    /* show test file names */
    n = sizeof(testfiles)/sizeof(testfiles[0]);
    printf("Serial test files are:\n");
    for (i=0; i<n; i++){
        printf("   %s\n", testfiles[i]);
    }

    if(dowrite) {
        printf("testing HDF5 dataset write...\n");
        for(i = 0; i < n; i++)
        hdf5writeAll(testfiles[i]);
    }
    if(doread) {
        printf("testing HDF5 dataset read...\n");
        hdf5readAll(testfiles[1]);
    }

    if (!(dowrite || doread)){
        usage();
        nerrors++;
    }

finish:
    if (nerrors)
        printf("***H5Tuner tests detected %d errors***\n", nerrors);
    else{
        printf("===================================\n");
        printf("H5Tuner Write tests finished with no errors\n");
        printf("===================================\n");
    }
    if (docleanup)
        cleanup();

    return(nerrors);
}


