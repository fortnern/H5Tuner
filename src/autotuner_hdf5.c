
/*
* Copyright by The HDF Group.
* All rights reserved.
*
* This file is part of h5tuner. The full h5tuner copyright notice,
* including terms governing use, modification, and redistribution, is
* contained in the file COPYING, which can be found at the root of the
* source code distribution tree.  If you do not have access to this file,
* you may request a copy from help@hdfgroup.org.
*/

#include "autotuner_private.h"
#define __USE_GNU
#include <dlfcn.h>

#define FORWARD_DECL(name, ret, args) \
    ret (*__fake_ ## name)args = NULL;

#define DECL(__name) __name

#define MAP_OR_FAIL(func) \
    if(!(__fake_ ## func)) \
    { \
        __fake_ ## func = dlsym(RTLD_NEXT, #func); \
        if(!(__fake_ ## func)) { \
            fprintf(stderr, "H5Tuner failed to map symbol: %s\n", #func); \
            exit(1); \
        } \
    }

/* MSC - Needs a test */
herr_t set_gpfs_parameter(mxml_node_t *tree, const char *parameter_name, const char *filename, /* OUT */ char **new_filename)
{
    const char *node_file_name;
    const char *file_basename;
    mxml_node_t *node;
    herr_t ret_value = SUCCEED;

    /* Retrieve base name for filename */
    if(NULL == (file_basename = strrchr(filename, '/')))
        file_basename = filename;
    else
        file_basename++;

    for(node = mxmlFindElement(tree, tree, parameter_name, NULL, NULL, MXML_DESCEND);
            node != NULL; node = mxmlFindElement(node, tree, parameter_name, NULL, NULL, MXML_DESCEND)) {
        node_file_name = mxmlElementGetAttr(node, "FileName");
#ifdef DEBUG
        printf("Node_file_name: %s\n", node_file_name);
#endif
        if(!node_file_name || !strcmp(file_basename, node_file_name)) {
            if(!strcmp(parameter_name, "IBM_lockless_io")) {
            printf("Node_file_name: %s\n", node_file_name);
                if(!strcmp(node->child->value.text.string, "true")) {
                    /* to prefix the filename with "bglockless:". */
                    if(NULL == (*new_filename = (char *)malloc(sizeof(char) * (strlen(filename) + sizeof("bglockless:")))))
                        ERROR("Unable to allocate new filename");

                    strcpy(*new_filename, "bglockless:");
                    strcat(*new_filename, filename);
                }

                if(node_file_name)
                    break;
            }
            else
                ERROR("Unknown GPFS parameter");
        }
    }

done:
    return ret_value;
}


herr_t set_mpi_parameter(mxml_node_t *tree, const char *parameter_name, const char *filename, MPI_Info *orig_info)
{
    const char *node_file_name;
    mxml_node_t *node;
    herr_t ret_value = SUCCEED;

    for(node = mxmlFindElement(tree, tree, parameter_name, NULL, NULL,MXML_DESCEND);
            node != NULL; node = mxmlFindElement(node, tree, parameter_name, NULL, NULL,MXML_DESCEND)) {
        node_file_name = mxmlElementGetAttr(node, "FileName");

#ifdef DEBUG
        /* printf("Node_file_name: %s\n", node_file_name); */
#endif
        /* MSC - CHECK usage of strstr().. need to switch to strcmp() probably */
        if(!node_file_name || strstr(filename, node_file_name)) {
            if(MPI_Info_set(*orig_info, parameter_name, node->child->value.text.string) != MPI_SUCCESS)
                ERROR("Failed to set MPI info");

            if(node_file_name)
                break;
        }
    }

done:
    return ret_value;
}

/*(tree, "chunk", "D1", space_id);

set_hdf5_fcreate_parameter(file_name, ...)
set_hdf5_dcreate_parameter(file_name, dataset_name, ...);*/


hid_t set_fapl_parameter(mxml_node_t *tree, const char *parameter_name, const char *filename, hid_t fapl_id)
{
    const char *node_file_name;
    mxml_node_t *node;
    herr_t ret_value = SUCCEED;
    hid_t dcpl_id=-1;

    for(node = mxmlFindElement(tree, tree, parameter_name, NULL, NULL,MXML_DESCEND);
            node != NULL; node = mxmlFindElement(node, tree, parameter_name, NULL, NULL,MXML_DESCEND)) {
        node_file_name = mxmlElementGetAttr(node, "FileName");
#ifdef DEBUG
          /* printf("Node_file_name: %s\n", node_file_name); */
#endif
        /* MSC - strstr() wrong usage here */
        if(!node_file_name || strstr(filename, node_file_name))  {
#ifdef DEBUG
            /* printf("H5Tuner: setting %s: %s\n", parameter_name, node->child->value.text.string); */
#endif
            if(!strcmp(parameter_name, "sieve_buf_size")) {
                if(H5Pset_sieve_buf_size(fapl_id, atoi(node->child->value.text.string)) < 0)
                    ERROR("Unable to set sieve buffer size");

                if(node_file_name)
                    break;
            }
            else if(!strcmp(parameter_name, "alignment")) {
                char *threshold = strtok(node->child->value.text.string, ",");
                char *alignment = strtok(NULL, ",");

                if(threshold == NULL)
                    ERROR("Unable to read alignment threshold");
                if(alignment == NULL)
                    ERROR("Unable to read alignment");

#ifdef DEBUG
                /* printf("H5Tuner: setting Threshold=%s; Alignment=%s\n", threshold, alignment); */
#endif
                if(H5Pset_alignment(fapl_id, (hsize_t)strtoll(threshold, NULL, 10), (hsize_t)strtoll(alignment, NULL, 10)) < 0)
                    ERROR("Unable to set alignment");

                if(node_file_name)
                    break;
            }
            else
                ERROR("Unknown FAPL parameter");
        }
    }

done:
    return ret_value;
}

/* MSC - This is not tested with chunk and will no get in to chunk setting with H5Dcreate() */
herr_t set_dcpl_parameter(mxml_node_t *tree, const char *parameter_name, const char *filename, const char *variable_name, hid_t space_id, hid_t dcpl_id)
{
    const char *node_file_name;
    mxml_node_t *node;
    hsize_t *dims = NULL;
    hsize_t *chunk_arr = NULL;
    herr_t ret_value = SUCCEED;

    for(node = mxmlFindElement(tree, tree, parameter_name, NULL, NULL,MXML_DESCEND);
            node != NULL; node = mxmlFindElement(node, tree, parameter_name, NULL, NULL,MXML_DESCEND)) {
        node_file_name = mxmlElementGetAttr(node, "FileName");
#ifdef DEBUG
          /* printf("Node_file_name: %s\n", node_file_name); */
#endif
        /* MSC - strstr() wrong usage here */
        if(!node_file_name || strstr(filename, node_file_name))  {
#ifdef DEBUG
            /* printf("H5Tuner: setting %s: %s\n", parameter_name, node->child->value.text.string); */
#endif
            if(!strcmp(parameter_name, "chunk")) {
                const char* node_variable_name = mxmlElementGetAttr(node, "VariableName");
#ifdef DEBUG
                printf("H5Tuner: VariableName: %s\n", node_variable_name);
#endif
                if(!node_variable_name || (!strcmp(node_variable_name, variable_name))) {
                    char *chunk_dim_str;
                    long long chunk_dim_ll;
                    int ndims;
                    int i;

                    if((ndims = H5Sget_simple_extent_ndims(space_id)) < 0)
                        ERROR("Unable to get number of space dimensions");
                    if(NULL == (dims = (hsize_t *)malloc(sizeof(hsize_t) * ndims)))
                        ERROR("Unable to allocate array of dimensions");
                    if(H5Sget_simple_extent_dims(space_id, dims, NULL) < 0)
                        ERROR("Unable to get space dimensions");
#ifdef DEBUG
    /* printf("dims[0] = %d, ndims = %d\n", dims[0], ndims);
    printf("dims[1] = %d, ndims = %d\n", dims[1], ndims);
    printf("dims[2] = %d, ndims = %d\n", dims[2], ndims); */
#endif

                    if(NULL == (chunk_arr = (hsize_t *)malloc(sizeof(hsize_t) * ndims)))
                        ERROR("Unable to allocate array of chunk dimensions");

                    if(NULL == (chunk_dim_str = strtok(node->child->value.text.string, ",")))
                        ERROR("Unable to find chunk dimension in attribute string");
                    chunk_dim_ll = strtoll(chunk_dim_str, NULL, 10);
                    if(chunk_dim_ll == LLONG_MAX)
                        ERROR("Chunk dimension overflowed");
                    if(chunk_dim_ll <= 0)
                        ERROR("Invalid chunk dimension");
                    chunk_arr[0] = (hsize_t)chunk_dim_ll;
#ifdef DEBUG
    /* printf("H5Tuner: Setting chunk[0] for %s -> %d\n", filename, chunk_arr[0]); */
#endif
                    for(i = 1; i < ndims; i++) {
                        if(NULL == (chunk_dim_str = strtok(NULL, ",")))
                            ERROR("Unable to find chunk dimension in attribute string");
                        chunk_dim_ll = strtoll(chunk_dim_str, NULL, 10);
                        if(chunk_dim_ll == LLONG_MAX)
                            ERROR("Chunk dimension overflowed");
                        if(chunk_dim_ll <= 0)
                            ERROR("Invalid chunk dimension");
                        chunk_arr[i] = (hsize_t)chunk_dim_ll;
                        #ifdef DEBUG
                          /* printf("H5Tuner: Setting chunk[%d] for %s -> %d\n", i, filename, chunk_arr[i]); */
                        #endif
                    }

                    H5Pset_chunk(dcpl_id, ndims, chunk_arr);

                    if(node_file_name && node_variable_name)
                        break;
                }
            }
            else
                ERROR("Unknown DCPL parameter");
        }
    }

done:
    free(dims);
    dims = NULL;

    return ret_value;
}


FORWARD_DECL(H5Fcreate, hid_t, (const char *filename, unsigned flags, hid_t fcpl_id, hid_t fapl_id));
FORWARD_DECL(H5Dwrite, herr_t, (hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t xfer_plist_id, const void * buf));
FORWARD_DECL(H5Dcreate1, hid_t, (hid_t loc_id, const char *name, hid_t type_id, hid_t space_id, hid_t dcpl_id));
FORWARD_DECL(H5Dcreate2, hid_t, (hid_t loc_id, const char *name, hid_t dtype_id, hid_t space_id, hid_t lcpl_id, hid_t dcpl_id, hid_t dapl_id));


hid_t DECL(H5Fcreate)(const char *filename, unsigned flags, hid_t fcpl_id, hid_t fapl_id)
{
    hid_t ret_value = -1;
    herr_t ret = -1;
    MPI_Comm new_comm = MPI_COMM_NULL;
    MPI_Info new_info = MPI_INFO_NULL;
    FILE *fp = NULL;
    mxml_node_t *tree;
    char *new_filename = NULL;
    char file_path[1024];
    hid_t real_fapl_id = -1;
    hid_t driver;

    MAP_OR_FAIL(H5Fcreate);

    strcpy(file_path, "config.xml");
    /* char *config_file = getenv("AT_CONFIG_FILE"); */
    /* char *file_path = config_file ; */
  #ifdef DEBUG
      /* printf("\nH5Tuner: Loading parameters file: %s\n", file_path); */
  #endif

    if(NULL == (fp = fopen(file_path, "r")))
        ERROR("Unable to open config file");
    if(NULL == (tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK)))
        ERROR("Unable to load config file");

    /* Set up/copy FAPL */
    if(fapl_id == H5P_DEFAULT) {
        if((real_fapl_id = H5Pcreate(H5P_FILE_ACCESS)) < 0)
            ERROR("Unable to create FAPL");
    }
    else if((real_fapl_id = H5Pcopy(fapl_id)) < 0)
        ERROR("Unable to copy FAPL");

    if((driver = H5Pget_driver(real_fapl_id)) < 0)
        ERROR("Unable to get file driver");

    if(driver == H5FD_MPIO) {
    #ifdef DEBUG
          /* printf("Driver is H5FD_MPIO\n"); */
        #endif
        if(H5Pget_fapl_mpio(real_fapl_id, &new_comm, &new_info) < 0)
            ERROR("Unable to get MPIO file driver info");
    }
    else
        ERROR("H5Tuner Library supports mpio drivers only()");

    if(new_info == MPI_INFO_NULL) {
#ifdef DEBUG
          /* printf("H5Tuner: found no MPI_Info object and is creating a new one.\n"); */
#endif
        if(MPI_Info_create(&new_info) != MPI_SUCCESS)
            ERROR("Unable to create MPI info");
    }
    else {

#ifdef DEBUG
          /* printf("H5Tuner: MPI_Info object is not null. Continuing.\n"); */
#endif
    }

#ifdef DEBUG
    {
      int nkeys = -1;
      if(MPI_Info_get_nkeys(new_info, &nkeys) != MPI_SUCCESS)
        ERROR("Unable to get number of MPI keys");
      /* printf("H5Tuner: MPI_Info object has %d keys\n", nkeys); */
    }
#endif

    if(set_gpfs_parameter(tree, "IBM_lockless_io", filename, &new_filename) < 0)
        ERROR("Unable to set GPFS parameter \"IBM_lockless_io\"");
    if(set_mpi_parameter(tree, "IBM_largeblock_io", filename, &new_info) < 0)
        ERROR("Unable to set MPI parameter \"IBM_largeblock_io\"");

    if(set_mpi_parameter(tree, "striping_factor", filename, &new_info) < 0)
        ERROR("Unable to set MPI parameter \"striping_factor\"");
    if(set_mpi_parameter(tree, "striping_unit", filename, &new_info) < 0)
        ERROR("Unable to set MPI parameter \"striping_unit\"");

    if(set_mpi_parameter(tree, "cb_buffer_size", filename, &new_info) < 0)
        ERROR("Unable to set MPI parameter \"cb_buffer_size\"");
    if(set_mpi_parameter(tree, "cb_nodes", filename, &new_info) < 0)
        ERROR("Unable to set MPI parameter \"cb_nodes\"");
    if(set_mpi_parameter(tree, "bgl_nodes_pset", filename, &new_info) < 0)
        ERROR("Unable to set MPI parameter \"bgl_nodes_pset\"");

    if(set_fapl_parameter(tree, "sieve_buf_size", filename, real_fapl_id) < 0)
        ERROR("Unable to set FAPL parameter \"sieve_buf_size\"");
    if(set_fapl_parameter(tree, "alignment", filename, real_fapl_id) < 0)
        ERROR("Unable to set FAPL parameter \"alignment\"");

#ifdef DEBUG
    {
      int nkeys = -1;
      if(MPI_Info_get_nkeys(new_info, &nkeys) != MPI_SUCCESS)
        ERROR("Unable to get number of MPI keys");
      /* printf("H5Tuner: completed parameters setting \n");
      printf("H5Tuner created MPI_Info object has %d keys!\n", nkeys); */
    }
#endif

    assert(driver == H5FD_MPIO);
    if(H5Pset_fapl_mpio(real_fapl_id, new_comm, new_info) < 0)
        ERROR("Unable to set MPI file driver");

#ifdef DEBUG
      /* printf("\nH5Tuner: calling H5Fcreate.\n"); */
#endif
    ret_value = __fake_H5Fcreate(new_filename ? new_filename : filename, flags, fcpl_id, real_fapl_id);

done:
    if(fp && (fclose(fp) != 0))
        DONE_ERROR("Failure closing config file");

    free(new_filename);
    new_filename = NULL;

    /*if((new_comm != MPI_COMM_NULL) && (MPI_Comm_free(&new_comm) != MPI_SUCCESS))
        DONE_ERROR("Failure freeing MPI comm");
    if((new_info != MPI_INFO_NULL) && (MPI_Info_free(&new_info) != MPI_SUCCESS))
        DONE_ERROR("Failure freeing MPI info");*/

    if((ret_value < 0) && (real_fapl_id >= 0) && (H5Pclose(real_fapl_id) < 0))
        DONE_ERROR("Failure closing FAPL");
    real_fapl_id = -1;

    return ret_value;
}

herr_t DECL(H5Dwrite)(hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t xfer_plist_id, const void * buf) {
    herr_t ret = -1;

    MAP_OR_FAIL(H5Dwrite);

#ifdef DEBUG
    /* printf("dataset_id: %d\n", dataset_id);
      printf("mem_type_id: %d\n", mem_type_id);
      printf("mem_space_id: %d\n", mem_space_id);
      printf("file_space_id: %d\n", file_space_id);
      printf("xfer_plist_id: %d\n", xfer_plist_id); */
#endif

#ifdef DEBUG
      /* printf("\nH5Tuner: calling H5Dwrite.\n"); */
#endif
    ret = __fake_H5Dwrite(dataset_id, mem_type_id, mem_space_id, file_space_id, xfer_plist_id, buf);

    return ret;
}


hid_t prepare_dcpl(hid_t loc_id, const char *name, hid_t space_id, hid_t dcpl_id)
{
    FILE *fp = NULL;
    mxml_node_t *tree;
    char file_path[1024];
    char *h5_filename = NULL;
    ssize_t h5_filename_len;
    hid_t copied_dcpl_id = -1;
    hid_t ret_value = -1;

    strcpy(file_path, "config.xml");

    if(NULL == (fp = fopen(file_path, "r")))
        ERROR("Unable to open config file");
    if(NULL == (tree = mxmlLoadFile(NULL, fp, MXML_TEXT_CALLBACK)))
        ERROR("Unable to load config file");

    /* Get file name */
    if((h5_filename_len = H5Fget_name(loc_id, NULL, 0)) < 0)
        ERROR("Unable to get HDF5 file name length");
    if(NULL == (h5_filename = malloc((size_t)h5_filename_len + 1)))
        ERROR("Unable to allocate HDF5 file name buffer");
    if(H5Fget_name(loc_id, h5_filename, (size_t)h5_filename_len + 1) < 0)
        ERROR("Unable to get HDF5 file name");

    /* Set up/copy DCPL */
    if(dcpl_id == H5P_DEFAULT) {
        if((copied_dcpl_id = H5Pcreate(H5P_DATASET_CREATE)) < 0)
            ERROR("Unable to create DCPL");
    }
    else if((copied_dcpl_id = H5Pcopy(dcpl_id)) < 0)
        ERROR("Unable to copy DCPL");

    if(set_dcpl_parameter(tree, "chunk", h5_filename, name, space_id, copied_dcpl_id) < 0)
        ERROR("Unable to set DCPL parameter \"chunk\"");

    ret_value = copied_dcpl_id;

done:
    if(fp && (fclose(fp) != 0))
        DONE_ERROR("Failure closing config file");

    free(h5_filename);
    h5_filename = NULL;

    if((ret_value < 0) && (copied_dcpl_id >= 0) && (H5Pclose(copied_dcpl_id) < 0))
        DONE_ERROR("Failure closing DCPL");
    copied_dcpl_id = -1;

    return ret_value;
}


hid_t DECL(H5Dcreate1)(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id, hid_t dcpl_id) {
    hid_t real_dcpl_id = -1;
    hid_t ret_value = -1;

    MAP_OR_FAIL(H5Dcreate1);

#ifdef DEBUG
      /* printf("\nH5Tuner: Loading parameters file for H5Dcreate1: %s\n", file_path); */
#endif

    /* Get real DCPL */
    if((real_dcpl_id = prepare_dcpl(loc_id, name, space_id, dcpl_id)) < 0)
        ERROR("Unable to obtain real DCPL");

#ifdef DEBUG
      /* printf("\nH5Tuner: calling H5Dcreate1.\n"); */
  #endif

    ret_value = __fake_H5Dcreate1(loc_id, name, type_id, space_id, real_dcpl_id);

done:
    if((real_dcpl_id >= 0) && (H5Pclose(real_dcpl_id) < 0))
        DONE_ERROR("Failure closing DCPL");
    real_dcpl_id = -1;

    return ret_value;
}

hid_t DECL(H5Dcreate2)(hid_t loc_id, const char *name, hid_t dtype_id, hid_t space_id, hid_t lcpl_id, hid_t dcpl_id, hid_t dapl_id) {
    hid_t real_dcpl_id = -1;
    hid_t ret_value = -1;

    MAP_OR_FAIL(H5Dcreate2);

#ifdef DEBUG
      /* printf("\nH5Tuner: Loading parameters file for H5Dcreate2: %s\n", file_path); */
#endif

    /* Get real DCPL */
    if((real_dcpl_id = prepare_dcpl(loc_id, name, space_id, dcpl_id)) < 0)
        ERROR("Unable to obtain real DCPL");

#ifdef DEBUG
      /* printf("\nH5Tuner: calling H5Dcreate2.\n"); */
 #endif

    ret_value = __fake_H5Dcreate2(loc_id, name, dtype_id, space_id, lcpl_id, real_dcpl_id, dapl_id);

done:
    if((real_dcpl_id >= 0) && (H5Pclose(real_dcpl_id) < 0))
        DONE_ERROR("Failure closing DCPL");
    real_dcpl_id = -1;

    return ret_value;
}

