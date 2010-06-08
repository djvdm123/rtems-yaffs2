/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2010 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * This simple implementation of a name-value store assumes a small number of values and fits
 * into a small finite buffer.
 *
 * Each attribute is stored as a record:
 *  sizeof(int) bytes   record size.
 *  strnlen+1 bytes name null terminated.
 *  nbytes    value.
 *  ----------
 *  total size  stored in record size 
 */


#include "yaffs_nameval.h"

#include "yportenv.h"
 
static int nval_find(const char *xb, int xb_size, const char *name)
{
	int pos=0;
	int size;

	memcpy(&size,xb,sizeof(int));
	while(size > 0 && (size < xb_size) && (pos + size < xb_size)){
		if(strncmp(xb+pos+sizeof(int),name,size) == 0)
			return pos;
		pos += size;
		if(pos < xb_size -sizeof(int))
			memcpy(&size,xb + pos,sizeof(int));
		else
			size = 0;
	}
	return -1;
}

static int nval_used(const char *xb, int xb_size)
{
	int pos=0;
	int size;

	memcpy(&size,xb + pos,sizeof(int));
	while(size > 0 && (size < xb_size) && (pos + size < xb_size)){
		pos += size;
		if(pos < xb_size -sizeof(int))
			memcpy(&size,xb + pos,sizeof(int));
		else
			size = 0;
	}
	return pos;
}

int nval_del(char *xb, int xb_size, const char *name)
{
	int pos  = nval_find(xb, xb_size, name);
	int size;
	
	if(pos >= 0 && pos < xb_size){
		/* Find size, shift rest over this record, then zero out the rest of buffer */
		memcpy(&size,xb+pos,sizeof(int));
		memcpy(xb + pos, xb + pos + size, xb_size - (pos + size));
		memset(xb + (xb_size - size),0,size);
		return 0;
	} else
		return -ENOENT;
}

int nval_set(char *xb, int xb_size, const char *name, const char *buf, int bsize, int flags)
{
	int pos = nval_find(xb,xb_size,name);
	int namelen = strnlen(name,xb_size);
	int reclen;

	if(flags & NVAL_CREATE && pos >= 0)
		return -EEXIST;
	if(flags & NVAL_REPLACE && pos < 0)
		return -ENOENT;

	nval_del(xb,xb_size,name);

	pos = nval_used(xb, xb_size);
	
	if(pos < xb_size && bsize < xb_size && namelen < xb_size){
		reclen = (sizeof(int) + namelen + 1 + bsize);
		if( pos + reclen < xb_size){
			memcpy(xb + pos,&reclen,sizeof(int));
			pos +=sizeof(int);
			strncpy(xb + pos, name, reclen);
			pos+= (namelen+1);
			memcpy(xb + pos,buf,bsize);
			pos+= bsize;
			return 0;
		}
	}
	return -ENOSPC;
}

int nval_get(const char *xb, int xb_size, const char *name, char *buf, int bsize)
{
	int pos = nval_find(xb,xb_size,name);
	int size;
	
	if(pos >= 0 && pos< xb_size){
		
		memcpy(&size,xb +pos,sizeof(int));
		pos+=sizeof(int); /* advance past record length */
		size -= sizeof(int);

		/* Advance over name string */
		while(xb[pos] && size > 0 && pos < xb_size){
			pos++;
			size--;
		}
		/*Advance over NUL */
		pos++;
		size--;

		if(size <= bsize){
			memcpy(buf,xb + pos,size);
			return size;
		}
		
	}
	return -ENOENT;
}

int nval_list(const char *xb, int xb_size, char *buf, int bsize)
{
	int pos = 0;
	int size;
	int name_len;
	int ncopied = 0;
	int filled = 0;

	memcpy(&size,xb + pos,sizeof(int));
	while(size > sizeof(int) && size <= xb_size && (pos + size) < xb_size && !filled){
		pos+= sizeof(int);
		size-=sizeof(int);
		name_len = strnlen(xb + pos, size);
		if(ncopied + name_len + 1 < bsize){
			memcpy(buf,xb+pos,name_len);
			buf+= name_len;
			*buf = '\0';
			buf++;
			ncopied += (name_len+1);
		} else
			filled = 1;
		pos+=size;
		if(pos < xb_size -sizeof(int))
			memcpy(&size,xb + pos,sizeof(int));
		else
			size = 0;
	}
	return ncopied;
}

int nval_load(char *xb, int xb_size, const char *src, int src_size)
{
	int tx_size;
	int used;
	
	tx_size = xb_size;
	if(tx_size > src_size)
		tx_size = src_size;

	memcpy(xb,src,tx_size);
	used = nval_used(xb, xb_size);
	
	if( used < xb_size)
		memset(xb+ used, 0, xb_size - used);
	return used;	
}

int nval_save(const char *xb, int xb_size, char *dest, int dest_size)
{
	int tx_size;
	
	tx_size = xb_size;
	if(tx_size > dest_size)
		tx_size = dest_size;

	memcpy(dest,xb,tx_size);
	return tx_size;
}
