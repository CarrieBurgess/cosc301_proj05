/*
The prestigous and awesome authors: Carrie and Shreeya
The date of glorious completion: COLGATE DAY 13 Dec 2013
The reasoning behind this wonderous project: required.  And to make a virtualalized directory/ file system that looks localized to the user


How to cycle through key/objects in s3: say want /x/y
-- first get root directory, find if there is an x directory (as know full path).  then go to /x, cycle through to make sure y doesn't exist, make new dirent, write to s3, update/ write /x to s3


SHREEYA!!!   I tried to block off everything I modified, so if one or both of us is decoding, you can just comment out the appropriate chunks.  Just make sure that there is a "return -EIO" at the end of the function if it needs to return something when checking.

Things I likely missed:
-freeing everything
-assigning all the components of the s3dirent_t
-error message (especially with the outputs for get_object and put_object)
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
 
 
 //Carrie and Shreeya
void *fs_init(struct fuse_conn_info *conn)
{
    fprintf(stderr, "fs_init --- initializing file system.\n");
    s3context_t *ctx = GET_PRIVATE_DATA;
    /////////////////Carrie and Shreeya
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
    ssize_t rv = s3fs_put_object(ctx->s3bucket, "/", (uint8_t*)root, sizeof(root));	//changed 1st entry from S3BUCKET
    if (rv < 0) {
        printf("Failure in s3fs_put_object\n");
    } 
    else if (rv < root[0].size) {
        printf("Failed to upload full test object (s3fs_put_object %zd)\n", rv);
    } 
    else {
        printf("Successfully put test object in s3 (s3fs_put_object)\n");
    }
    //free(root[0].name);
    free(root);
    return ctx; //****What does this do???
    ////////////////////////Carrie and Shreeya
}

/*
 * Clean up filesystem -- free any allocated data.
 * Called once on filesystem exit.
 */
void fs_destroy(void *userdata) {
    fprintf(stderr, "fs_destroy --- shutting down file system.\n");
    free(userdata);
}

 //Carrie written function
int is_there(s3dirent_t obj, const char * name) {
	if(strcmp(obj.name, name)==0) {
		return 1; //found a match!
	}
	else {
		return 0; //didn't find a match...
	}
}


/* 
 * Get file attributes.  Similar to the stat() call
 * (and uses the same structure).  The st_dev, st_blksize,
 * and st_ino fields are ignored in the struct (and 
 * do not need to be filled in).
 */

int fs_getattr(const char *path, struct stat *statbuf) {
//should return somthing on success
    fprintf(stderr, "fs_getattr(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;  
    
    ///////////////Carrie code
    //int rv;
    //first need to check if it is a file or directory:
    char * dircpy = strdup(path); //copying so I don't change the original
    char* dir = dirname(dircpy);
    uint8_t *buffer = NULL;
    //ssize_t rv = s3fs_get_object(ctx->s3bucket, dir, &buffer, 0, 0);
	s3fs_get_object(ctx->s3bucket, dir, &buffer, 0, 0);
    int size = sizeof(buffer)/sizeof(s3dirent_t);
    s3dirent_t* buf = (s3dirent_t*)malloc(sizeof(s3dirent_t)*size); //malloced
    buf = (s3dirent_t *)buffer; //filled in?
    int i =0;
    for(; i<size; i++) {
    	if(strcmp(buf[i].name, path)==0) {
    		break;
    	}
    }
    struct stat *st = malloc(sizeof(struct stat));
    if(buf[i].type=='f') { //it was a file, metadata stored here:
    	st->st_mode = buf[i].mode;
//    	stat->st_nlink = NULL;
    	st->st_uid = buf[i].uid;
    	st->st_gid = buf[i].gid;
//    	stat->st_rdev = NULL;
    	st->st_size = (off_t)buf[i].size;
//    	stat->st_blocks = NULL;
    	st->st_atime = buf[i].access_time;
//    	stat->st_mtime = NULL;
//    	stat->st_ctime = NULL;
    }
    else if(buf[i].type=='d') { //was a directory, go find metadata there
    	uint8_t *buffer2= NULL;
    	s3fs_get_object(ctx->s3bucket, path, &buffer2, 0, sizeof(s3dirent_t)); //only gets first entry, '.'
    	s3dirent_t* tmp = (s3dirent_t*)malloc(sizeof(s3dirent_t)*1); //malloced
    	tmp = (s3dirent_t *)buffer2;
    	st->st_mode = tmp[0].mode;
//    	stat->st_nlink = NULL;
    	st->st_uid = tmp[0].uid;
    	st->st_gid = tmp[0].gid;
//    	stat->st_rdev = NULL;
    	st->st_size = (off_t)tmp[0].size;
//    	stat->st_blocks = NULL;
    	st->st_atime = tmp[0].access_time;
//    	stat->st_mtime = NULL;
//    	stat->st_ctime = NULL;
		free(tmp);						//freed
    }
    free(buf);
	statbuf = st;
	free(st);
    return 0;
    /////////////////end of Carrie written
}


/*
 * Open directory
 *
 * This method should check if the open operation is permitted for
 * this directory
 */
 
 
 /////////////Carrie written function
int dir_exists_rec(const char* path, s3context_t *ctx) {	
//return -1 on error, 0 if successfully found directory
	char* copy_path1 = strdup(path);
	char* dir = dirname(copy_path1);
	if(!strcmp(path,"/") || (path==NULL) || !strcmp(dir,".")) {
		return 0;
	}	
	int err = dir_exists_rec(dir, ctx);
	if(err<0) {
		printf("Well, something went wrong.\n");
		return -1;
	}	
	//int rv;
	uint8_t *buffer = NULL;
    s3fs_get_object(ctx->s3bucket, dir, &buffer, 0, 0);  //1st entry was S3BUCKET
    int size = sizeof(buffer)/sizeof(s3dirent_t);
    s3dirent_t* buf = (s3dirent_t*)malloc(sizeof(s3dirent_t)*size); //malloced
    buf = (s3dirent_t *)buffer; //filled in?
	int i = 0;
	int j = 0;
	for(; i<size; i++) {
		j = is_there(buf[i], path);
		if(j==1) {
			free(buf);
			return 0;
		} 
	}
	free(buf);
	if(j==0) {
		return -1;
	}
	return -EIO;
} 

 
int fs_opendir(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_opendir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    ///////////////Carrie written
    int check = dir_exists_rec(path, ctx); 
    if(check==0) {
    	return 0;
    } 
    ////////////////////
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
    
    //////////////////////Carrie written code
    
    ssize_t rv;
	uint8_t *buffer = NULL;
    rv = s3fs_get_object(ctx->s3bucket, path, &buffer, 0, 0);  //1st entry was S3BUCKET
    if(rv<0) {
    	return -EIO;
    }
    int size = sizeof(buffer)/sizeof(s3dirent_t);
    s3dirent_t* storage = (s3dirent_t*)malloc(sizeof(s3dirent_t)*size); //malloced
    storage = (s3dirent_t *)buffer; //filled in?
	int i = 0;
	//int j = 0;
	for(; i<size; i++) {
		 if(filler(buf, storage[i].name, NULL, 0) !=0) {
			free(storage);
		 	return -ENOMEM;
		 }
	}
    ////*****************I am not sure how offset and *fi are supposed to be used....?
	free(storage);
    //////////////////////
    return 0;
}


/*
 * Release directory.
 */
int fs_releasedir(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_releasedir(path=\"%s\")\n", path);
    //s3context_t *ctx = GET_PRIVATE_DATA;
    
    ////////////////////////Carrie 
    //return -EIO;
    return 0; //he said don't do anything besides return success... 0 is success here....
    ////////////////////////
}


/* 
 * Create a new directory.
 *
 * Note that the mode argument may not have the type specification
 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
 * correct directory type bits (for setting in the metadata)
 * use mode|S_IFDIR.
 */
 
 
////////Carrie and Shreeya written function
int rec_check(const char *path, mode_t mode, s3context_t *ctx) {  
//return -1 on error, 0 if successfully added directory

//	char *dir;
//	strcpy(dir, path);
//	dirname(dir);
//	char *dir = dirname(path);
	
	char* copy_path1 = strdup(path);
//	char* copy_path2 = strdup(path);
	char* dir = dirname(copy_path1);

	if(!strcmp(path,"/") || (path==NULL) || !strcmp(dir,".")) {
		return 0;
	}

	
//	char *base = basename(path);
	int err = rec_check(dir, mode, ctx);
	if(err<0) {
		printf("Well, something went wrong.\n");
		return -1;
	}	
//	uint8_t ** buf; //buf is the base directory.  I think this is malloced.  
//	s3fs_get_object(S3BUCKET, dir, buf, 0, 0);
	
	int rv;
	
	uint8_t *buffer = NULL;
    // zeroes as last two args means that we want to retrieve entire object
    rv = (int) s3fs_get_object(ctx->s3bucket, dir, &buffer, 0, 0);  //1st entry was S3BUCKET
    int size = sizeof(buffer)/sizeof(s3dirent_t);
    s3dirent_t* buf = (s3dirent_t*)malloc(sizeof(s3dirent_t)*size); //malloced
    buf = (s3dirent_t *)buffer; //filled in?
	if(rv<0) {  //error!
		printf("An error occured retrieving the object from the cloud.\n");
		free(buf);
		return -1;
	}
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
			free(buf);			
			return 0;
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
    rv = s3fs_put_object(ctx->s3bucket, path, (uint8_t*)new, sizeof(new));  //1st entry was S3BUCKET
    if (rv < 0) {
        printf("Failure in s3fs_put_object\n");
    } 
    else if (rv < new[0].size) {
        printf("Failed to upload full test object (s3fs_put_object %d)\n", rv);
    } 
    else {
        printf("Successfully put test object in s3 (s3fs_put_object)\n");
    }
    //free(new[0].name); 		//freed
    free(new);				//freed

	//now update current directory
	if(unused==1) { //a previous directory was 'deleted', so we can use the space	
		strcpy(buf[loc_unused].name, path); //malloced
	    buf[loc_unused].type = 'd';
	    //rest of metadata in other key's '.'
   	 	ssize_t rv2 = s3fs_put_object(ctx->s3bucket, dir, (uint8_t*)buf, sizeof(buf)); //1st entry was S3BUCKET
    	if (rv2 < 0) {
       	 	printf("Failure in s3fs_put_object\n");
    	} 
    	else if (rv2 < ((int) buf[0].size)) {
        	printf("Failed to upload full test object (s3fs_put_object %zd)\n", rv2);
    	} 
    	else {
        	printf("Successfully put test object in s3 (s3fs_put_object)\n");
    	}
    	//free(buf[loc_unused].name);			//freed
    	//made new key, updated current cd: all done!	
	}
	else {
		//so there wasn't an unused space....
		s3dirent_t* new_curdir = malloc(sizeof(s3dirent_t)*(size+1)); //malloc
		i=0;
		for(; i<size; i++) { //rewriting info already there
			new_curdir[i] = buf[i]; //copy over what is there
		}	
		
		strcpy(new_curdir[i+1].name, path); //malloced
		new_curdir[i+1].type = 'd';
		//rest of metadata in new key's '.'
		rv = s3fs_put_object(ctx->s3bucket, dir, (uint8_t*)new_curdir, sizeof(new_curdir)); //1st entry was S3BUCKET
		if (rv < 0) {
		    printf("Failure in s3fs_put_object\n");
		} 
		else if (rv < new[0].size) {
		    printf("Failed to upload full test object (s3fs_put_object %d)\n", rv);
		} 
		else {
		    printf("Successfully put test object in s3 (s3fs_put_object)\n");
		}
		//free(new_curdir[i+1].name);
		free(new_curdir);
	}	
    free(copy_path1);					//freed
    free(buf);							//freed
	return 0;	
}
 
 
int fs_mkdir(const char *path, mode_t mode) {
    fprintf(stderr, "fs_mkdir(path=\"%s\", mode=0%3o)\n", path, mode);
    s3context_t *ctx = GET_PRIVATE_DATA;
    mode |= S_IFDIR; //permissions for all directories besides root
    
    ///////////Carrie
    rec_check(path, mode, ctx);
    ///////////
    /*
    
    
    make recursive function to call dirname starting at base, make sure exists.  If does, go one level up.  Either that or for loop.
    
    After checking that path up to new dir exists, and that new dir name isn't already at current path, then make new key/value thingamajig, put that in outer space.  Then go through, make parent dir +1 (num dirent = size/ sizeof(s3dirent_t) for how many already there), then remalloc all over and add in extra info at end.  Put in outer space, free shtuff
    
    
    */
    
    
    
    return -EIO;
}


/*
 * Remove a directory. Only removes if only entry is '.'.  Removes from parent as well
 */
int fs_rmdir(const char *path) {
    fprintf(stderr, "fs_rmdir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    
    /////////////////Carrie
    
    int check = dir_exists_rec(path, ctx); //verify exists;
    if(check==-1) {
    	printf("I'm sorry, it doesn't appear as though the written path exists.\n");
    }
    char* copy_path1 = strdup(path);
	char* dir = dirname(copy_path1); //so this now has the parent name
	int rv;
	uint8_t *buffer = NULL;
    rv = s3fs_get_object(ctx->s3bucket, path, &buffer, 0, 0);  //1st entry was S3BUCKET
    if(rv==-1) {
    	return -EIO;
    }
    int size = sizeof(buffer)/sizeof(s3dirent_t);
    if(size != sizeof(s3dirent_t)) { //there is not only 1 entry, so cannot remove
    	printf("This directory is not empty.  Please empty the contents of the directory.\n");
    	return -1;
    }
    //if got past that, there IS only 1 entry, which would be '.'
    int isrm = s3fs_remove_object(ctx->s3bucket, path);
    if(isrm==-1) {
    	printf("Error in removing object.\n");
    	return -1;
    }
    else {
    	printf("Object successfully removed from bucket.\n");
    }
    //now need to make it go away for the parent, dir
    uint8_t *buffer2 = NULL;
    rv = s3fs_get_object(ctx->s3bucket, dir, &buffer2, 0, 0);  //1st entry was S3BUCKET
    if(rv==-1) {
    	return -EIO;
    }
    int size2 = sizeof(buffer2)/sizeof(s3dirent_t);
    s3dirent_t* buf = (s3dirent_t*)malloc(sizeof(s3dirent_t)*size2); //malloced
    buf = (s3dirent_t *)buffer2; //filled in?
	int i = 0;
	int j = 0;
	for(; i<size2; i++) {
		if(is_there(buf[i], path)==1) {
			buf[i].type = 'u'; //just set it to unused so it won't be 'seen'
			j=1;
			break;
		} 
	}
	free(copy_path1);
	free(buf);
	if(j==0) {
		printf("For some reason, the directory couldn't be cleared from the parent directory.\n");
		return -EIO;
	}
	return 0;    
    //return -EIO;
    ////////////////////
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
    //s3context_t *ctx = GET_PRIVATE_DATA;
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
    //s3context_t *ctx = GET_PRIVATE_DATA;
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
   // s3context_t *ctx = GET_PRIVATE_DATA;
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
   // s3context_t *ctx = GET_PRIVATE_DATA;
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
  //  s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Rename a file.
 */
int fs_rename(const char *path, const char *newpath) {
    fprintf(stderr, "fs_rename(fpath=\"%s\", newpath=\"%s\")\n", path, newpath);
  //  s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Remove a file.
 */
int fs_unlink(const char *path) {
    fprintf(stderr, "fs_unlink(path=\"%s\")\n", path);
  //  s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}
/*
 * Change the size of a file.
 */
int fs_truncate(const char *path, off_t newsize) {
    fprintf(stderr, "fs_truncate(path=\"%s\", newsize=%d)\n", path, (int)newsize);
  //  s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Change the size of an open file.  Very similar to fs_truncate (and,
 * depending on your implementation), you could possibly treat it the
 * same as fs_truncate.
 */
int fs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_ftruncate(path=\"%s\", offset=%d)\n", path, (int)offset);
 //   s3context_t *ctx = GET_PRIVATE_DATA;
    return -EIO;
}


/*
 * Check file access permissions.  For now, just return 0 (success!)
 * Later, actually check permissions (don't bother initially).
 */
int fs_access(const char *path, int mask) {
    fprintf(stderr, "fs_access(path=\"%s\", mask=0%o)\n", path, mask);
  //  s3context_t *ctx = GET_PRIVATE_DATA;
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
