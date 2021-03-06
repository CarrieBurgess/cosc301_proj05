/*
authors: Carrie and Shreeya
The date of completion: COLGATE DAY 13 Dec 2013
project: to make a virtualalized directory/ file system that looks localized to the user


Professor Sommers:
Hardly anything works.  Carrie's computer: virtual machine refuses to test/ work properly.  Shreeya's computer: after clearing everything/ freeing memory, the virtual machine could connect to the internet via firefox, but the terminal refused to do so. 

Summary: we couldn't get anything to work.  Physics kept demanding that we couldn't dedicate the time
to camp out in a computer lab in McGregory to work on this.  So we could never really test it.  As a 
consequence, part 1 doesn't even work.  It is close, but we can't find the error, becuase
we can't debug.  We basically did pseudo-code for some functions in part 2 (after giving up on
our computers for part 1), so... you can get the concept.

Really sorry.  We tried valiantly.  Please forgive the lack of completeness.

-Carrie and Shreeya
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


 //Carrie and Shreeya
 /*
 This file is designed to clear the bucket, then initialize the root directory, "/", and
 put that on the s3 cloud
 */
void *fs_init(struct fuse_conn_info *conn)
{
    fprintf(stderr, "fs_init --- initializing file system.\n");
    s3context_t *ctx = GET_PRIVATE_DATA;

    s3fs_clear_bucket(S3BUCKET);
    s3dirent_t root = {'d', ".", 0, 0, 0, 0, 0, 0};

    root.mode = (S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR);
    root.size = sizeof(root);
    root.uid = getuid();
    root.gid = getgid();
    time_t t;
    time(&t);
    root.access_time = t;
    ssize_t rv = s3fs_put_object(ctx->s3bucket, "/", (uint8_t*)(&root), sizeof(root));	//changed 1st entry from S3BUCKET
    if (rv < 0) {
        printf("---------------------------------Failure in s3fs_put_object\n");
    } 

    return ctx; 

}

/*
 * Clean up filesystem -- free any allocated data.
 * Called once on filesystem exit.
 */
void fs_destroy(void *userdata) {
    fprintf(stderr, "fs_destroy --- shutting down file system.\n");
    free(userdata);
}

 //Carrie
 /*
 This function purely takes an input s3dirent_t and checks if the name inside matches the
 name given
 */
int is_there(s3dirent_t obj, const char * name) {
	if(strcmp(obj.name, name)==0) {
		return 1; //found a match!
	}
	else {
		return 0; //didn't find a match...
	}
}

 //Carrie and Shreeya
 /*
 This function looks at the parent directory of the path to find out
 if the path is to a directory or a file.  If it is to a file, the metadata
 is local/ in the parent directory, so it is obtained and stored in a struct stat*,
 which is later assigned to statbuf and passed back to the function that calls it.
 If the path is to a directory, then that means it has its own key in s3 space.  We
 retrieve that key and check the first entry, '.', where the metadata for the directory
 is stored, then store that in statbuf.
 This does not obtain all the information that stat() calls for, but gets the majority.
 (We are explicitely told to ignore st_dev, st_blksize, st_ino)
 */
int fs_getattr(const char *path, struct stat *statbuf) {
//should return somthing on success
    fprintf(stderr, "fs_getattr(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;  
	printf("\nget attribute\n");    
  
	printf("~~~~~~~~~~~`the path is current: %s\n", path);
    char * dircpy = strdup(path); //copying so I don't change the original
    char* dir = dirname(dircpy);
    s3dirent_t *buf = NULL;
    int rv = (int)s3fs_get_object(ctx->s3bucket, dir, (uint8_t**)&buf, 0, 0);
    int size = rv/sizeof(s3dirent_t);
	if(rv<0) {
		printf("Error obtaining object.\n");
		free(dircpy);
		return -1;
	}
    int i =0;                              
	printf("Got the object.  About to go find correct object in dir key, and size= %d\n",size);
	int check = 0;
    for(; i<size; i++) {
	 	printf("%s =? %s\n", buf[i].name, path);
    	if(strcmp(buf[i].name, path)==0) {
			check = 1;
    		break;
    	}
    }
	if (check) { // found a match
		printf("----------found match %s = %s\n", buf[i].name, path);
	}
	else printf("no match found!\n");

    struct stat *st = malloc(sizeof(struct stat));
    if(buf[i].type=='f') { //it was a file, metadata stored here:
		printf("entered if statement for file\n");    	
		st->st_mode = buf[i].mode;
    	st->st_uid = buf[i].uid;
    	st->st_gid = buf[i].gid;
	  	st->st_rdev = 0;
    	st->st_size = (off_t)buf[i].size;
    	st->st_blocks = 1;
    	st->st_atime = buf[i].access_time;
	   	st->st_mtime = 0;
    	st->st_ctime = 0;
    }
    else if(buf[i].type=='d') { //was a directory, go find metadata there
		printf("entered elseif statement for directory\n");
		printf("~~~~~~~~~~~~~~~~~~~~~the path is currently: %s\n", path);		
		s3dirent_t *tmp = NULL;
    	int rv = (int)s3fs_get_object(ctx->s3bucket, dir, (uint8_t**)&tmp, 0, 0);
//   		int size = rv/sizeof(s3dirent_t);
		if(rv<0) {
			printf("Error obtaining object.\n");
			free(dircpy);
			return -1;
		}
    	st->st_mode = tmp[0].mode;
    	st->st_nlink = 1;
    	st->st_uid = tmp[0].uid;
    	st->st_gid = tmp[0].gid;
    	st->st_rdev = 0;
    	st->st_size = (off_t)tmp[0].size;
    	st->st_blocks = 0;
    	st->st_atime = tmp[0].access_time;
    	st->st_mtime = 0;
    	st->st_ctime = 0;
		free(tmp);						//freed
    }
	printf("Well that was fun.  Something prolly happened.\n");
    free(buf);
	statbuf = st;
	free(st);
printf("\n------------------------get attribute end\n");    
    return 0;
    /////////////////end of Carrie written
}


/*
 * Open directory
 *
 * This method should check if the open operation is permitted for
 * this directory
 */
 
 
 //Carrie and Shreeya
 /*
 This function is designed to ensure a directory exists.  It works recursively, using
 the dirname() function call to slowly break up the path name until we reach the root
 directory.  Once there, we ensure that exists, and that there is a reference to the 
 directory one level up.  If so, we return, then the next level is checked, then the 
 next, etc.
 Ex: if trying to see if /x/y/z exists, we first check if the key "/" exists, and contains an
 object with the name "/x".  If so, then we find the key "/x" and see if it has an object
 with the name "/x/y".  If so, we look for a key with "/x/y" and see if it contains "/x/y/z",
 then finally check if the key "/x/y/z" exists.  
 */
int dir_exists_rec(const char* path, s3context_t *ctx) {
printf("\ndir exist recursive\n");    	
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
	int rv;
	s3dirent_t *buf = NULL;
    rv = (int)s3fs_get_object(ctx->s3bucket, dir, (uint8_t**)&buf, 0, 0);
    int size = rv/sizeof(s3dirent_t);
	if (rv<0) {
		printf("ERROR! couldn't get Object\n");
		free(copy_path1);
		return -1;
	}
    //s3dirent_t* buf = (s3dirent_t*)malloc(sizeof(s3dirent_t)*size); //malloced
    //buf = (s3dirent_t *)buffer; //filled in?
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
	printf("\ndir exist recursive RETURNS\n");    	
	return -EIO;
} 

/*
This literally just checks if the directory is there (and all of the subdirectories)
*/ 
int fs_opendir(const char *path, struct fuse_file_info *fi) {
	printf("\nOPEN DIR\n");    	
    fprintf(stderr, "fs_opendir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    ///////////////Carrie written
    int check = dir_exists_rec(path, ctx); 
    if(check==0) {
    	return 0;
    } 
    else {
    	return -1;
    }
    ////////////////////
	printf("\nOPEN DIR END\n");    	
    return -EIO;
}


/*  Carrie and Shreeya
This reads in the directory from the given path (assuming it exists) then uses the filler
function to read it in, as the project description indicates.
 */
int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
         struct fuse_file_info *fi)  {
	printf("\nREAD DIR\n");    	
    fprintf(stderr, "fs_readdir(path=\"%s\", buf=%p, offset=%d)\n", path, buf, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;
    
    //////////////////////Carrie written code    
    s3dirent_t *storage = NULL;
    int rv = (int)s3fs_get_object(ctx->s3bucket, path, (uint8_t**)&storage, 0, 0);
    int size = rv/sizeof(s3dirent_t);
    if(rv<0) {
    	return -EIO;
    }
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
	printf("\nREAD DIR END\n");    	
    return 0;
}


/*
 * Release directory.
 */
int fs_releasedir(const char *path, struct fuse_file_info *fi) {
printf("\nRELEASE DIR\n");    	
    fprintf(stderr, "fs_releasedir(path=\"%s\")\n", path);
    //s3context_t *ctx = GET_PRIVATE_DATA;
    
    ////////////////////////Carrie 
    //return -EIO;
	printf("\nRELEASE DIR END\n");    	
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
/*
rec_check operates a lot like dir_exists_rec except that, instead of just checking if a directory
exists, if it does not find the directory, it creates it.
Example: if you want /x/y, and only /x exists, then it recursively breaks it down to the key
"/", checks that it contains the object "/x", then gets the key "/x", sees there is no object
"/x/y", creats a key "/x/y", and adds the object "/x/y" to the key "/x".
*/

int rec_check(const char *path, mode_t mode, s3context_t *ctx) {  
printf("\nREC CHECK\n");     //////////////////////////HELPER FOR MKDIR   	
//return -1 on error, 0 if successfully added directory
	char* copy_path1 = strdup(path);
	char* dir = dirname(copy_path1);
	if(!strcmp(path,"/") || (path==NULL) || !strcmp(dir,".")) {
		return 0;
	}
	int err = rec_check(dir, mode, ctx);
	if(err<0) {
		printf("---Well, something went wrong IN THE REC_CHECK.\n");
		return -1;
	}	
	s3dirent_t *buf = NULL;
    int ret_v = (int)s3fs_get_object(ctx->s3bucket, dir, (uint8_t**)&buf, 0, 0); //*buf = parent
    int size = ret_v/sizeof(s3dirent_t);
	if(ret_v<0) {  //error!
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
	s3dirent_t new ={'d', ".", 0, 0, 0, 0, 0, 0}; 


    //strcpy(new[0].name, "."); //malloced
    new.mode = mode;
    new.size = sizeof(new);
    new.uid = getuid();
    new.gid = getgid();
    time_t t;
    time(&t);
    new.access_time = t;
    int rv = s3fs_put_object(ctx->s3bucket, path, (uint8_t*)&new, sizeof(new));
    //child obj created, put on s3
    if (rv < 0) {
        printf("Failure in s3fs_put_object\n");
    } 
    //free(new[0].name); 		//freed
    //free(new);				//freed
    
	//now update current directory
	if(unused==1) { //a previous directory was 'deleted', so we can use the space	
		strcpy(buf[loc_unused].name, path); //malloced
	    buf[loc_unused].type = 'd';
	    //rest of metadata in other key's '.'
	    int rv2 = s3fs_put_object(ctx->s3bucket, dir, (uint8_t*)buf, ret_v);
	    
	    //I don't know why we had a get_object here...we already got dir, now
	    //should put it back updated
    //  int rv2 = (int)s3fs_get_object(ctx->s3bucket, dir, (uint8_t**)&buf, 0, 0);
   	//  int size = rv/sizeof(s3dirent_t);
    	if (rv2 < 0) {
       	 	printf("Failure in s3fs_put_object\n");
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
			memcpy(new_curdir+i, buf+i, sizeof(s3dirent_t));
			//new_curdir[i] = buf[i]; //copy over what is there
		}		
		strcpy(new_curdir[i+1].name, path); //malloced
		new_curdir[i+1].type = 'd';
   		new_curdir[i+1].mode = 0;
    	new_curdir[i+1].size = 0;
    	new_curdir[i+1].uid = 0;
    	new_curdir[i+1].gid = 0;
    	new_curdir[i+1].dev = 0;
    	time_t t;
    	time(&t);
    	new_curdir[i+1].access_time = 0;
		rv = s3fs_put_object(ctx->s3bucket, dir, (uint8_t*)new_curdir, sizeof(new));
		//putting updated dir on cloud, with key overwritting old parent directory key
 	  	if (rv < 0) {
        	printf("Failure in s3fs_put_object\n");
    	} 
		//rest of metadata in new key's '.'
		// s3dirent_t *buffer = NULL;
		
    	/*rv = (int)s3fs_get_object(ctx->s3bucket, path, (uint8_t**)&new_curdir, 0, 0);
   	 //int size = rv/sizeof(s3dirent_t);
		if (rv < 0) {
		    printf("Failure in s3fs_put_object\n");
		} 
		else if (rv < new_curdir[0].size) {
		    printf("Failed to upload full test object (s3fs_put_object %d)\n", rv);
		} 
		*/
		//free(new_curdir[i+1].name);
		free(new_curdir);
	}	
    free(copy_path1);					//freed
    free(buf);							//freed
printf("\nREC CHANGE END\n");    	
	return 0;	
}
 
 /*
 This just calls the recursive function above.
 */
int fs_mkdir(const char *path, mode_t mode) {
printf("\nMKDIR\n");    	
    fprintf(stderr, "fs_mkdir(path=\"%s\", mode=0%3o)\n", path, mode);
    s3context_t *ctx = GET_PRIVATE_DATA;
    mode |= S_IFDIR; 

    int check = rec_check(path, mode, ctx);
	if(check!=0) {
		return -EIO;
	}
	else {
		return 0;
	}
    return -EIO;
}


/*
 * Remove a directory. Only removes if only entry is '.'.  Removes from parent as well.
 */
int fs_rmdir(const char *path) {
    fprintf(stderr, "fs_rmdir(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    
    /////////////////
    
    int check = dir_exists_rec(path, ctx); //verify exists;
    if(check==-1) {
    	printf("I'm sorry, it doesn't appear as though the written path exists.\n");
    }
    char* copy_path1 = strdup(path);
	char* dir = dirname(copy_path1); //so this now has the parent name

	s3dirent_t *buf = NULL;
    int rv = (int)s3fs_get_object(ctx->s3bucket, path, (uint8_t**)&buf, 0, 0);
    int size = rv/sizeof(s3dirent_t);
    if(rv==-1) {
    	return -EIO;
    }
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
    
    s3dirent_t *buf2 = NULL;
    int rv2 = (int)s3fs_get_object(ctx->s3bucket, dir, (uint8_t**)&buf2, 0, 0); //get parent
    int size2 = rv2/sizeof(s3dirent_t);
    if(rv2==-1) {
    	return -EIO;
    }
	int i = 0;
	int j = 0;
	for(; i<size2; i++) { //find child/ path/ directory that deleted
		if(is_there(buf2[i], path)==1) {
			buf2[i].type = 'u'; //just set it to unused so it won't be 'seen'
			j=1;
			break;
		} 
	}
	//put updated dir back in s3
	int rv3 = s3fs_put_object(ctx->s3bucket, dir, (uint8_t*)buf2, rv2); 
 	if (rv3 < 0) {
        printf("Failure in s3fs_put_object\n");
    } 
	free(copy_path1);
	free(buf);
	free(buf2);
	if(j==0) {
		printf("For some reason, the directory couldn't be cleared from the parent directory.\n");
		return -EIO;
	}
printf("\nMKDIR ENDDDD\n");    	
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
 
 //Carrie
int fs_mknod(const char *path, mode_t mode, dev_t dev) {
    fprintf(stderr, "fs_mknod(path=\"%s\", mode=0%3o)\n", path, mode);
    char* copy_path1 = strdup(path);
	char* dir = dirname(copy_path1);    
    s3context_t *ctx = GET_PRIVATE_DATA;
    s3dirent_t *buf = NULL;
    int ret_v = (int)s3fs_get_object(ctx->s3bucket, dir, (uint8_t**)&buf, 0, 0); //*buf = parent
    int size = ret_v/sizeof(s3dirent_t);
	if(ret_v<0) {  //error!
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
	int a = s3fs_put_object(ctx->s3bucket, path, (uint8_t*)buf, 0); //so for file, just
	//putting empty object into space.  
	
	//now, to update parent directory:
	if(unused==1) { //a previous object was 'deleted', so we can use the space	
		strcpy(buf[loc_unused].name, path); //malloced
	    buf[loc_unused].type = 'f';
	    buf[loc_unused].mode = mode;
	    buf[loc_unused].size = 0;
	    buf[loc_unused].uid = getuid();
	    buf[loc_unused].gid = getgid();
	    time_t t;
	    time(&t);
	    buf[loc_unused].access_time = t;
	    buf[loc_unused].dev = dev;
	    int b = s3fs_put_object(ctx->s3bucket, dir, (int8_t**)buf, ret_v);
		if(b<0) {
			printf("Failure to put object on cloud\n");
		}	
	}	
	else {
		//so there wasn't an unused space....
		s3dirent_t* new_curdir = NULL;
		i=0;
		for(; i<size; i++) { //rewriting info already there
			new_curdir[i] = buf[i]; //copy over what is there
		}		
		strcpy(new_curdir[i+1].name, path); //malloced
		new_curdir[i+1].type = 'f';
   		new_curdir[i+1].mode = mode;
    	new_curdir[i+1].size = ret_v/sizeof(s3dirent_t);
    	new_curdir[i+1].uid = getuid();
    	new_curdir[i+1].gid = getgid();
    	time_t t;
    	time(&t);
    	new_curdir[i+1].access_time = t;
    	new_curdir[i+1].dev = dev;
		int rv = s3fs_put_object(ctx->s3bucket, dir, (uint8_t*)new_curdir, (a+sizeof(s3dirent_t)));
		//putting updated dir on cloud, with key overwritting old parent directory key
 	  	if (rv < 0) {
        	printf("Failure in s3fs_put_object\n");
    	} 
    	free(new_curdir);
	}	
    free(copy_path1);					//freed
    free(buf);				
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
//shreeya
int fs_open(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_open(path\"%s\")\n", path);

    s3context_t *ctx = GET_PRIVATE_DATA;
////////////////	
	char * dircpy = strdup(path);
    char* dir = dirname(dircpy);
    s3dirent_t *buf = NULL;
    int rv = (int)s3fs_get_object(ctx->s3bucket, dir, (uint8_t**)&buf, 0, 0);
	if(rv<0) {
		printf("File is not in the directory.\n");
		free(dircpy);
		return -1;
	}
 
    int size = rv/sizeof(s3dirent_t);
    int i =0; 
	int check = 0;                             
    for(; i<size; i++) {
	 	//printf("%s =? %s\n", buf[i].name, path);
    	if(strcmp(buf[i].name, path)==0) {
			check = 1;
    		break;
    	}
    }
	if (check) { //file is there but still check for attribute
		if (buf[i].type=='f') {
			free(dircpy);
			return 0; //is a file 
		}
	}
	free(dircpy);
////////////////
    return -EIO;
}


/* 
 * Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  
 */
 //Carrie
int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_read(path=\"%s\", buf=%p, size=%d, offset=%d)\n",
          path, buf, (int)size, (int)offset);
    s3context_t *ctx = GET_PRIVATE_DATA;
    int ret_v = (int)s3fs_get_object(ctx->s3bucket, path, (uint8_t**)&buf, offset, (offset+size)); 
    if(ret_v<0) {
    	printf("You make me sad.\n");
    }
    return 0;
    return -EIO;
}


/*
 * Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.
 *///shreeya
int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_write(path=\"%s\", buf=%p, size=%d, offset=%d)\n",
  		    path, buf, (int)size, (int)offset);
  	s3context_t *ctx = GET_PRIVATE_DATA;
//////

	char * dircpy = strdup(path);
    char* dir = dirname(dircpy);
    s3dirent_t *buff = NULL;
    int rv = (int)s3fs_get_object(ctx->s3bucket, dir, (uint8_t**)&buff, 0, 0);
	if(rv<0) {
		printf("File is not in the directory.\n");
		free(dircpy);
		return -1;
	}
    int s = rv/sizeof(s3dirent_t);
    int i =0; 
	int check = 0;                             
    for(; i<s; i++) {
	 	//printf("%s =? %s\n", buf[i].name, path);
    	if(strcmp(buff[i].name, path)==0) {
			check = 1;
    		break;
    	}
    }
	if (check) { //file is there but still check for attribute
		if (buff[i].type=='f') {
			free(dircpy);
		//file is here read it now.....
		
		int omg = 1;
		int start = 0;
		char *str = NULL;
		while (omg>0) {
			omg = s3fs_get_object(ctx->s3bucket, dir, (uint8_t**)&str, start, 1024);
			start = start + omg; //find total #of bytes
		}  
		if(offset>(start+1)) return -1; //cannot add beyond EOF
		start = start + (int)size; //total size to be pushed into the file
		omg = 1;
		char * file_buffer = malloc(sizeof(char)*start);
		start = 0;
		int offset_check = 0;
		while (omg>0) { //read file again to assign it to the 
			omg = s3fs_get_object(ctx->s3bucket, dir, (uint8_t**)&str, start, 1024);
///////////////////////////come back here
			start = start + omg;
			if (!offset_check) {
				if (start<(int)offset-1024) { //will have to read until the offset then copy in all the buf then move along with rest of file content
					int offset_size = offset%1024;
					strncpy(file_buffer,str,(offset_size));
					strcpy(file_buffer,buf); //write the file
					strncpy(file_buffer, str+offset_size*4, 1024-offset_size); //copy the rest
					offset_check = 1; //when
				}
			else strcpy(file_buffer, str);
			}
		}  
		
		if (!offset_check) strcpy(file_buffer, buf); //copy the write buffer at the end if have to attach something at the end
		//read each 1024 buffers and keep adding it to a giant buffer and then send the giant buffer to the put

			//ssize_t srv = s3fs_put_object(ctx->s3bucket, path, (uint8_t*)(buf), sizeof(buf));	//changed 1st entry from S3BUCKET
    //if (srv < 0) {
      //  printf("---------------------------------Failure in s3fs_put_object\n");
    //}    
			free(file_buffer);
			return 0; //put stuff on file
		}
	}
	free(dircpy);
	
//////	
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
    return 0;
}


/*
 * Rename a file.
 */
 //Carrie
int fs_rename(const char *path, const char *newpath) {
    fprintf(stderr, "fs_rename(fpath=\"%s\", newpath=\"%s\")\n", path, newpath);
    s3context_t *ctx = GET_PRIVATE_DATA;
 	s3dirent_t *buf = NULL;
  	int ret_v = (int)s3fs_get_object(ctx->s3bucket, path, (uint8_t*)&buf, 0, 0); 
    if(ret_v<0) {
    	printf("You make me sad.\n");
    }
    int ret2 = s3fs_put_object(ctx->s3bucket, newpath, (uint8_t*)&buf, (ssize_t)ret_v);
    if(ret2<0) {
    	printf("NNOOOOOOOO!!!!!\n");
    }
    // put on new path
    s3fs_remove_object(ctx->s3bucket, path);
    char* copy_path1 = strdup(path);
	char* dir = dirname(copy_path1);
	s3dirent_t *buf2 = NULL;
	int ret_v2 = (int)s3fs_get_object(ctx->s3bucket, dir, (uint8_t**)&buf2, 0, 0);
	if(ret_v2<0) {
		printf("Once upon a time, there was a girl who was a physics major, and physics decided she shouldn't have time for her other required project in a not-physics course.  So.... pseudocode.\n");
	}
	int size2 = ret_v2/sizeof(s3dirent_t);
    if(ret_v2==-1) {
    	return -EIO;
    }
	int i = 0;
	for(; i<size2; i++) { //find child/ path/ directory that deleted
		if(is_there(buf2[i], path)==1) {
			strcpy(buf2[i].name, newpath);
			break;
		} 
	}
	//put updated dir back in s3
	int rv3 = s3fs_put_object(ctx->s3bucket, dir, (uint8_t*)buf2, ret_v2); 
 	if (rv3 < 0) {
        printf("Failure in s3fs_put_object\n");
    } 
    return 0;
    return -EIO;
}


/*
 * Remove a file.
 */
 //Carrie
int fs_unlink(const char *path) {
    fprintf(stderr, "fs_unlink(path=\"%s\")\n", path);
    s3context_t *ctx = GET_PRIVATE_DATA;
    s3fs_remove_object(ctx->s3bucket, path);
    char* copy_path1 = strdup(path);
	char* dir = dirname(copy_path1);
	s3dirent_t *buf2 = NULL;
	int ret_v2 = (int)s3fs_get_object(ctx->s3bucket, dir, (uint8_t**)&buf2, 0, 0);
	if(ret_v2<0) {
		printf("You know what's a good show?  Firefly.  I think I'll watch that over break.\n");
	}
	int size2 = ret_v2/sizeof(s3dirent_t);
    if(ret_v2==-1) {
    	return -EIO;
    }
	int i = 0;
	for(; i<size2; i++) { //find child/ path/ directory that deleted
		if(is_there(buf2[i], path)==1) {
			buf2[i].type = 'u';
			break;
		} 
	}
	//put updated dir back in s3
	int rv3 = s3fs_put_object(ctx->s3bucket, dir, (uint8_t*)buf2, ret_v2); 
 	if (rv3 < 0) {
        printf("Failure in s3fs_put_object\n");
    } 
    free(buf2);
    return 0;
}
/*
 * Change the size of a file.
 */
int fs_truncate(const char *path, off_t newsize) {
    fprintf(stderr, "fs_truncate(path=\"%s\", newsize=%d)\n", path, (int)newsize);
    s3context_t *ctx = GET_PRIVATE_DATA;
    s3dirent_t *buf2 = NULL;
	s3fs_get_object(ctx->s3bucket, path, (uint8_t**)&buf2, 0, 0);
	s3fs_put_object(ctx->s3bucket, path, (uint8_t*)buf2, newsize);
	free(buf2);
	return 0;
    return -EIO;
}


/*
 * Change the size of an open file.  Very similar to fs_truncate (and,
 * depending on your implementation), you could possibly treat it the
 * same as fs_truncate.
 */
int fs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "fs_ftruncate(path=\"%s\", offset=%d)\n", path, (int)offset);
  //  s3context_t *ctx = GET_PRIVATE_DATA;
 	fs_truncate(path, offset);
 	return 0;
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
