#include <stdio.h>
#include <sys/types.h>
#include <limits.h>

#include "run.h"
#include "util.h"

void *base = 0;
void *end = 0;
p_meta find_meta(p_meta *last, size_t size) {
  p_meta index = base;
  p_meta result = -1;
  if(base!=last){
  switch(fit_flag){
    case FIRST_FIT:
    {
      do{
        if(result == -1 && index->free && index->size>=size) result=index;
	if(result == -1 && !index->next) result=index;
        index = index->next;
      }while(index);
    }
    break;

    case BEST_FIT:
    {
      do{
	if(index->free && index->size>=size){
	  if(result==-1) result=index;
	  else if(result->size>index->size) result = index;
	}
	if(result == -1 && !index->next) result=index;
        index = index->next;
      }while(index);
    }
    break;

    case WORST_FIT:
    {
      do{
	if(index->free && index->size>=size){
	  if(result==-1) result=index;
	  else if(result->size < index->size) result = index;
	}
	if(result == -1 && !index->next) result=index;
        index = index->next;
      }while(index);
    }
    break;
  }
  }
  return result;
}

void *m_malloc(size_t size) {
  if(base == 0) {
    base = sbrk(0);
    end = base;
  }
  size = (size+3)/4*4;
  int length = size + META_SIZE;
  p_meta tar1 = find_meta(end,size);
  if(tar1 ==-1 || (!tar1->next && (!tar1->free || tar1->free && tar1->size<size))){
    p_meta tar2 = end;
    end += length;
    if(brk(end) == -1) return ((void*)0);
    tar2->next = 0;
    tar2->free = 0;
    tar2->prev = tar1;
    tar2->size = size;
    if(tar1!=-1) tar1->next = tar2;
    tar1 = tar2;
  }
  else{
    m_realloc(tar1->data,size);
  }
  return tar1->data;
}

void m_free(void *ptr) {
  p_meta cur = ptr-META_SIZE;
  cur->free = 1;
  if(cur->next && cur->next->free == 1) {
    cur->size +=cur->next->size + META_SIZE;
    cur->next = cur->next->next;
  }
  if(cur->prev!=-1){
    if(cur->prev->free){
      cur = cur->prev;
      cur->size +=cur->next->size + META_SIZE;
      cur->next = cur->next->next; 
    }
    if(!cur->next){
      end-=cur->size + META_SIZE;
      cur->prev->next = 0;
    }
  }
  else if(!cur->next && !cur->prev){
    end = base;
  }
  ptr = 0;
}

void *m_realloc(void* ptr, size_t size){
  p_meta cur = ptr-META_SIZE;
  size = (size+3)/4*4;
  if(cur->size == size) return ptr;
  else if(cur->size < size){
    if(cur->next && cur->next->free 
       && cur->size + cur->next->size + META_SIZE>=size){
      cur->size += cur->next->size + META_SIZE;
      cur->next = cur->next->next;
      cur->next->prev = cur;
      if(cur->size-size < META_SIZE){
        return ptr;
      }
      else{
        p_meta next = (int)cur + size + META_SIZE;
        next->prev = cur;
        next->next = cur->next;
        next->size = cur->size - size - META_SIZE;
        cur->next = next;
        cur->size = size;
        cur->free = 0;
	m_free(next->data);
        return cur->data;
      }
    }
    else {
      m_free(cur->data);
      void * new_ptr = m_malloc(size);
      strcpy(new_ptr, ptr);
      return new_ptr;
    }
  }
  else if(cur->size-size < META_SIZE){
    return ptr;
  }
  else{
    p_meta next = (int)cur + size + META_SIZE;
    next->prev = cur;
    next->next = cur->next;
    next->size = cur->size - size - META_SIZE;
    cur->next = next;
    cur->size = size;
    cur->free = 0;
    m_free(next->data);
    return cur->data;
  }
}
