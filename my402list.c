#include "cs402.h"
#include "my402list.h"
#include <stdio.h>
#include <stdlib.h>

int  My402ListLength(My402List* list)
{
	return list->num_members;
}

int  My402ListEmpty(My402List* list)
{
	return list->num_members<=0;
}

int  My402ListAppend(My402List* list, void* new_obj)
{
	My402ListElem* new_element=(My402ListElem*)malloc(sizeof(My402ListElem));
    if(new_element==NULL) return FALSE;
    else new_element->obj=new_obj;

	if(My402ListEmpty(list))
	{
        (list->anchor).next=new_element;
        (list->anchor).prev=new_element;
        new_element->next=&(list->anchor);
        new_element->prev=&(list->anchor);
	}
    else
    {
        ((list->anchor).prev)->next=new_element;
        new_element->prev=(list->anchor).prev;
        new_element->next=&(list->anchor);
        (list->anchor).prev=new_element;
    }
	list->num_members++;
    return TRUE;
}

int  My402ListPrepend(My402List* list, void* new_obj)
{
    My402ListElem* new_element=(My402ListElem*)malloc(sizeof(My402ListElem));
    if(new_element==NULL) return FALSE;
    else new_element->obj=new_obj;

	if(My402ListEmpty(list))
	{
        (list->anchor).next=new_element;
        (list->anchor).prev=new_element;
        new_element->next=&(list->anchor);
        new_element->prev=&(list->anchor);
	}
    else
    {
        new_element->prev=&(list->anchor);
        new_element->next=(list->anchor).next;
        ((list->anchor).next)->prev=new_element;
		(list->anchor).next=new_element;
    }
	list->num_members++;
    return TRUE;
}

void My402ListUnlink(My402List* list, My402ListElem* element)
{
    element->prev->next=element->next;
    element->next->prev=element->prev;
	list->num_members--;
    free(element);
}

void My402ListUnlinkAll(My402List* list)
{
    while(!My402ListEmpty(list))
	{
		My402ListUnlink(list, My402ListLast(list));
	}
}

int  My402ListInsertAfter(My402List* list, void* new_obj, My402ListElem* element)
{
    if(element==NULL)   return My402ListAppend(list, new_obj);
    else
    {
        My402ListElem* new_element=(My402ListElem*)malloc(sizeof(My402ListElem));
        if(new_element==NULL) return FALSE;
        else new_element->obj=new_obj;

        new_element->prev=element;
        new_element->next=element->next;
        element->next->prev=new_element;
        element->next=new_element;
		list->num_members++;
        return TRUE;
    }
}

int  My402ListInsertBefore(My402List* list, void* new_obj, My402ListElem* element)
{
    if(element==NULL)   return My402ListPrepend(list, new_obj);
    else
    {
        My402ListElem* new_element=(My402ListElem*)malloc(sizeof(My402ListElem));
        if(new_element==NULL) return FALSE;
        else new_element->obj=new_obj;

        new_element->prev=element->prev;
        new_element->next=element;
        element->prev->next=new_element;
        element->prev=new_element;
		list->num_members++;
        return TRUE;
    }
}

My402ListElem *My402ListFirst(My402List* list)
{
    if(My402ListEmpty(list)) return NULL;
    else return (list->anchor).next;
}

My402ListElem *My402ListLast(My402List* list)
{
    if(My402ListEmpty(list)) return NULL;
    else return (list->anchor).prev;
}

My402ListElem *My402ListNext(My402List* list, My402ListElem* element)
{
    if(element->next==&(list->anchor)) return NULL;
    else return element->next;
}

My402ListElem *My402ListPrev(My402List* list, My402ListElem* element)
{
    if(element->prev==&(list->anchor)) return NULL;
    else return element->prev;
}

My402ListElem *My402ListFind(My402List* list, void* target)
{
    My402ListElem* element=(list->anchor).next;
    while(element!=&(list->anchor))
    {
        if(element->obj==target) return element;
        element=element->next;
    }
    return NULL;
}

int My402ListInit(My402List* list)
{
	if(list==NULL) return FALSE;
	list->num_members=0;
	(list->anchor).obj=NULL;
	(list->anchor).next=&(list->anchor);
	(list->anchor).prev=&(list->anchor);
	return TRUE;
}

