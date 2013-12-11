/*
The prestigous and awesome authors: Carrie and Shreeya
The date of glorious completion: COLGATE DAY 13 Dec 2013
The reasoning behind this wonderous project: required.  And to make a virtualalized directory/ file system that looks localized to the user



QUESTION:  do we have to resize each directory every time we include a directory underneath it?  (are the children part of the metadata, so when we add a child file or directory, do we need to change the size of the parent)
-Don't even need size of directories.  IF want, though, don't need grandchildren per se.  If "/" has test.txt, ., and X, then would have size 3 (if wanted size)

QUESTION: do we destroy everything in the bucket for the destroy function?
--> use s3 clear bucket.  **For destroy, don't really need to do much beyond what is written.*  

SHOULDN'T HAVE GLOBAL VARIABLES.  

How to cycle through key/objects in s3: say want /x/y
-- first get root directory, find if there is an x directory (as know full path).  then go to /x, cycle through to make sure y doesn't exist, make new dirent, write to s3, update/ write /x to s3



*/





/* This code is based on the fine code written by Joseph Pfeiffer for his
   fuse system tutorial. */

#include "s3fs.h"
#include "libs3_wrapper.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define GET_PRIVATE_DATA ((s3context_t *) fuse_get_context()->private_data)

/*
 * For each function below, if you need to return an error,
 * read the appropriate man page for the call and see what
 * error codes make sense for the type of failure you want
 * to convey.  For example, many of the calls below return
 * -EIO (an I/O error), since there are no S3 calls yet
 * implemented.  (Note that you need to return the negative
 * value for an error code.)
 
 */

/* *************************************** */
/*        Stage 1 callbacks                */
/* *************************************** */

/*
 * Initialize the file system.  This is called once upon
 * file system startup.
 */
void *fs_init(struct fuse_conn_info *conn)
{
    fprintf(stderr, "fs_init --- initializing file system.\n");
    s3context_t *ctx = GET_PRIVATE_DATA;
    printf(S3BUCKET);
    s3fs_clear_bucket(S3BUCKET);
    s3dirent_t* root = malloc(sizeof(s3dirent_t)*1); //malloced
    strcpy(root[0].name, "."); //malloced
    root[0].type = 'd';
    root[0].mode = (S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR);
    root[0].size = sizeof(root);
    root[0].uid = getuid();
    root[0].gid = getgid();
    time_t t;
    time(&t);
    root[0].access_time = t;
    ssize_t rv = s3fs_put_object(S3BUCKET, "/", (uint8_t*)root, sizeof(root));
    if (rv < 0) {
        printf("Failure in s3fs_put_object\n");
    } 
    else if (rv < root[0].size) {
        printf("Failed to upload full test object (s3fs_put_object %d)\n", rv);
    } 
    else {
        printf("Successfully put test object in s3 (s3fs_put_object)\n");
    }
    free(root[0].name);
    free(root);
    return ctx; //****What does this do???
}

/*
 * Clean up filesystem -- free any allocated data.
 * Called once on filesystem exit.
 */
void fs_destroy(void *userdata) {
    fprintf(stderr, "fs_destroy --- shutting down file system.\n");
    free(userdata);
}


/* 
 * Get file attributes.  Similar to the stat() call
 * (and uses the same structure).  The st_dev, st_blksize,
 * and st_ino fields are ignored in the struct (and 
 * do not need to be filled in).
 */

int fs_getattr(const char *path, struct stat *statbuf) {
    fprintf(stderr, "fs_getattr(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Open directory
 *
 * This method should check if the open operation is permitted for
 * this directory
 */
int fs_opendir(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_opendir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Read directory.  See the project description for how to use the filler
 * function for filling in directory items.
 */
int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
         struct fuse_file_info *fi)
{
    fprintf(stderr, "fs_readdir(path=\"%s\", buf=%p, offset=%d)\n",
          path, buf, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Release directory.
 */
int fs_releasedir(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_releasedir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/* 
 * Create a new directory.
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits (for setting in the metadata)
 * use mode|S_IFDIR.
 */
 
 
int is_there(s3dirent_t obj, char * name) {
	if(strcmp(obj.name, name)==0) {
		return 1; //found a match!
	}
	else {
		return 0; //didn't find a match...
	}
} 

void rec_check(char *path, mode_t mode) {  
//return -1 on error, 1 if successfully added directory
	if((path == "/") || (path==NULL) || (dirname(path)==".")) {
		return;
	}
	char *dir = dirname(path);
//	char *base = basename(path);
	rec_check(dir, mode);	
	uint8_t ** buf; //buf is the base directory.  I think this is malloced.  
	ssize_t s3fs_get_object(S3BUCKET, dir, buf, 0, 0);
	(s3dirent_t)buf;
	int size = sizeof(buf)/sizeof(s3dirent_t);
	int i = 0;
	int j = 0;
	int unused = 0;
	int loc_unused = 0;
	for(; i<size; i++) {
		j = is_there(buf[i], path); 
		if(buf[i].type == 'u') {
			unused = 1;
			loc_unused = i;
		}
		if(j==1) { //found it, don't need to look any more
			return;
		}
	}
	//if got to this point, then object we are looking for doesn't exist/ we have to make it
	
	//make new key and obj
	s3dirent_t* new = malloc(sizeof(s3dirent_t)*1); //malloced
    strcpy(new[0].name, "."); //malloced
    new[0].type = 'd';
    new[0].mode = mode;
    new[0].size = sizeof(new);
    new[0].uid = getuid();
    new[0].gid = getgid();
    time_t t;
    time(&t);
    new[0].access_time = t;
    ssize_t rv = s3fs_put_object(S3BUCKET, path, (uint8_t*)new, sizeof(new));
    if (rv < 0) {
        printf("Failure in s3fs_put_object\n");
    } 
    else if (rv < new[0].size) {
        printf("Failed to upload full test object (s3fs_put_object %d)\n", rv);
    } 
    else {
        printf("Successfully put test object in s3 (s3fs_put_object)\n");
    }

	//now update current directory
	if(unused==1) { //a previous directory was 'deleted', so we can use the space	
		strcpy(buf[loc_unused].name, path); //malloced
	    buf[loc_unused].type = 'd';
	    //rest of metadata in other key's '.'
   	 	ssize_t rv = s3fs_put_object(S3BUCKET, dir, (uint8_t*)buf, sizeof(buf));
    	if (rv < 0) {
       	 	printf("Failure in s3fs_put_object\n");
    	} 
    	else if (rv < buf[0].size) {
        	printf("Failed to upload full test object (s3fs_put_object %d)\n", rv);
    	} 
    	else {
        	printf("Successfully put test object in s3 (s3fs_put_object)\n");
    	}
    	return; //made new key, updated current cd: all done!	
	}
	
	//so there wasn't an unused space....
	s3dirent_t* new_curdir = malloc(sizeof(s3dirent_t)*(size+1)); //malloc
	i=0;
	for(; i<size; i++) { //rewriting info already there
		new_curdir[i] = buf[i]; //copy over what is there
	}	
		
	strcpy(new_curdir[i+1].name, path); //malloced
    new_curdir[i+1].type = 'd';
    //rest of metadata in new key's '.'
    ssize_t rv = s3fs_put_object(S3BUCKET, dir, (uint8_t*)new_curdir, sizeof(new_curdir));
    if (rv < 0) {
        printf("Failure in s3fs_put_object\n");
    } 
    else if (rv < new[0].size) {
        printf("Failed to upload full test object (s3fs_put_object %d)\n", rv);
    } 
    else {
        printf("Successfully put test object in s3 (s3fs_put_object)\n");
    }	
	return;	
}
 
 
int fs_mkdir(const char *path, mode_t mode) {
    fprintf(stderr, "fs_mkdir(path=\"%s\", mode=0%3o)\n", path, mode);
    s3context_t *ctx = GET_PRIVATE_DATA;
    mode |= S_IFDIR; //permissions for all directories besides root
    rec_check(path, mode);
    /*
    
    
    make recursive function to call dirname starting at base, make sure exists.  If does, go one level up.  Either that or for loop.
    
    After checking that path up to new dir exists, and that new dir name isn't already at current path, then make new key/value thingamajig, put that in outer space.  Then go through, make parent dir +1 (num dirent = size/ sizeof(s3dirent_t) for how many already there), then remalloc all over and add in extra info at end.  Put in outer space, free shtuff
    
    
    */
    
    /*
    char * dirname = dirname(path); 
    int base_dir_exits = 0;
    //need to make this a loop through everything in bucket
    	uint8_t *retrieved_object = NULL;
    	// zeroes as last two args means that we want to retrieve entire object
    	rv = s3fs_get_object(S3BUCKET, S3SECRETKEY, &retrieved_object, 0, 0);
    	if (rv < 0) {
     	   printf("Failure in s3fs_get_object\n");
    	} else if (rv < object_length) {
    	    printf("Failed to retrieve entire object (s3fs_get_object %d)\n", rv);
    	} else {
    	    printf("Successfully retrieved test object from s3 (s3fs_get_object)\n");
        	if (strcmp((const char *)retrieved_object, test_object) == 0) {
        	    printf("Retrieved object looks right.\n");
  	      } else {
    	        printf("Retrieved object doesn't match what we sent?!\n");
    	    }
    	}
    	(s3dirent_t)retrieved_object;
    	
    	
    	if(retrieved_object->name == path) {
    		printf("Directory already exists at current path.\n");
    		return;
    	}
    	if(retrieved_object->name == dirname) {
    		if(retrieved_object->type == 'd') {
  	  			printf("Base directory exists: can make new directory at that location.\n");
    			base_dir_exists = 1;
    		}
    	}
    
    //end loop
    
    if(base_dir_exists == 0) {
    	printf("Base directory did not exist: cannot make new directory at that location.\n");
    	return;
    }
    
    
    
    
	s3dirent_t dir; // = malloc(sizeof(s3dirent_t)); //***Does this need to be malloced?
    strcpy(dir.name, path);
    strcpy(dir.parent, "/0"); //will have to remember to free these!!! Also might not need parent
    dir.type = 'd';
    dir.mode = mode;
    dir.size = sizeof(dir);
    ssize_t rv = s3fs_put_object(S3BUCKET, S3SECRETKEY, (uint8_t*)dir, dir.size);
    if (rv < 0) {
        printf("Failure in s3fs_put_object\n");
    } 
    else if (rv < dir.size) {
        printf("Failed to upload full test object (s3fs_put_object %d)\n", rv);
    } 
    else {
        printf("Successfully put test object in s3 (s3fs_put_object)\n");
    }
    */
    
    return -EIO;
}


/*
 * Remove a directory. 
 */
int fs_rmdir(const char *path) {
    fprintf(stderr, "fs_rmdir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/* *************************************** */
/*        Stage 2 callbacks                */
/* *************************************** */


/* 
 * Create a file "node".  When a new file is created, this
 * function will get called.  
 * This is called for creation of all non-directory, non-symlink
 * nodes.  You *only* need to handle creation of regular
 * files here.  (See the man page for mknod (2).)
 */
int fs_mknod(const char *path, mode_t mode, dev_t dev) {
    fprintf(stderr, "fs_mknod(path=\"%s\", mode=0%3o)\n", path, mode);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/* 
 * File open operation
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  
 * 
 * Optionally open may also return an arbitrary filehandle in the 
 * fuse_file_info structure (fi->fh).
 * which will be passed to all file operations.
 * (In stages 1 and 2, you are advised to keep this function very,
 * very simple.)
 */
int fs_open(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_open(path\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/* 
 * Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  
 */
int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_read(path=\"%s\", buf=%p, size=%d, offset=%d)\n",
          path, buf, (int)size, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.
 */
int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_write(path=\"%s\", buf=%p, size=%d, offset=%d)\n",
          path, buf, (int)size, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.  
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 */
int fs_release(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_release(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Rename a file.
 */
int fs_rename(const char *path, const char *newpath) {
    fprintf(stderr, "fs_rename(fpath=\"%s\", newpath=\"%s\")\n", path, newpath);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Remove a file.
 */
int fs_unlink(const char *path) {
    fprintf(stderr, "fs_unlink(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}
/*
 * Change the size of a file.
 */
int fs_truncate(const char *path, off_t newsize) {
    fprintf(stderr, "fs_truncate(path=\"%s\", newsize=%d)\n", path, (int)newsize);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Change the size of an open file.  Very similar to fs_truncate (and,
 * depending on your implementation), you could possibly treat it the
 * same as fs_truncate.
 */
int fs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_ftruncate(path=\"%s\", offset=%d)\n", path, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Check file access permissions.  For now, just return 0 (success!)
 * Later, actually check permissions (don't bother initially).
 */
int fs_access(const char *path, int mask) {
    fprintf(stderr, "fs_access(path=\"%s\", mask=0%o)\n", path, mask);
    s3context_t *ctx = GET_PRIVATE_DATA;
    return 0;
}


/*
 * The struct that contains pointers to all our callback
 * functions.  Those that are currently NULL aren't 
 * intended to be implemented in this project.
 */
struct fuse_operations s3fs_ops = {
  .getattr     = fs_getattr,    // get file attributes
  .readlink    = NULL,          // read a symbolic link
  .getdir      = NULL,          // deprecated function
  .mknod       = fs_mknod,      // create a file
  .mkdir       = fs_mkdir,      // create a directory
  .unlink      = fs_unlink,     // remove/unlink a file
  .rmdir       = fs_rmdir,      // remove a directory
  .symlink     = NULL,          // create a symbolic link
  .rename      = fs_rename,     // rename a file
  .link        = NULL,          // we don't support hard links
  .chmod       = NULL,          // change mode bits: not implemented
  .chown       = NULL,          // change ownership: not implemented
  .truncate    = fs_truncate,   // truncate a file's size
  .utime       = NULL,          // update stat times for a file: not implemented
  .open        = fs_open,       // open a file
  .read        = fs_read,       // read contents from an open file
  .write       = fs_write,      // write contents to an open file
  .statfs      = NULL,          // file sys stat: not implemented
  .flush       = NULL,          // flush file to stable storage: not implemented
  .release     = fs_release,    // release/close file
  .fsync       = NULL,          // sync file to disk: not implemented
  .setxattr    = NULL,          // not implemented
  .getxattr    = NULL,          // not implemented
  .listxattr   = NULL,          // not implemented
  .removexattr = NULL,          // not implemented
  .opendir     = fs_opendir,    // open directory entry
  .readdir     = fs_readdir,    // read directory entry
  .releasedir  = fs_releasedir, // release/close directory
  .fsyncdir    = NULL,          // sync dirent to disk: not implemented
  .init        = fs_init,       // initialize filesystem
  .destroy     = fs_destroy,    // cleanup/destroy filesystem
  .access      = fs_access,     // check access permissions for a file
  .create      = NULL,          // not implemented
  .ftruncate   = fs_ftruncate,  // truncate the file
  .fgetattr    = NULL           // not implemented
};



/* 
 * You shouldn't need to change anything here.  If you need to
 * add more items to the filesystem context object (which currently
 * only has the S3 bucket name), you might want to initialize that
 * here (but you could also reasonably do that in fs_init).
 */
int main(int argc, char *argv[]) {
    // don't allow anything to continue if we're running as root.  bad stuff.
    if ((getuid() == 0) || (geteuid() == 0)) {
    	fprintf(stderr, "Don't run this as root.\n");
    	return -1;
    }
    s3context_t *stateinfo = malloc(sizeof(s3context_t));
    memset(stateinfo, 0, sizeof(s3context_t));

    char *s3key = getenv(S3ACCESSKEY);
    if (!s3key) {
        fprintf(stderr, "%s environment variable must be defined\n", S3ACCESSKEY);
        return -1;
    }
    char *s3secret = getenv(S3SECRETKEY);
    if (!s3secret) {
        fprintf(stderr, "%s environment variable must be defined\n", S3SECRETKEY);
        return -1;
    }
    char *s3bucket = getenv(S3BUCKET);
    if (!s3bucket) {
        fprintf(stderr, "%s environment variable must be defined\n", S3BUCKET);
        return -1;
    }
    strncpy((*stateinfo).s3bucket, s3bucket, BUFFERSIZE);

    fprintf(stderr, "Initializing s3 credentials\n");
    s3fs_init_credentials(s3key, s3secret);

    fprintf(stderr, "Totally clearing s3 bucket\n");
    s3fs_clear_bucket(s3bucket);

    fprintf(stderr, "Starting up FUSE file system.\n");
    int fuse_stat = fuse_main(argc, argv, &s3fs_ops, stateinfo);
    fprintf(stderr, "Startup function (fuse_main) returned %d\n", fuse_stat);
    
    return fuse_stat;
}
