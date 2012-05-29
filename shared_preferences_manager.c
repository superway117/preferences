
#include <common.h>
#include "shared_preferences.h"
#include <arraylist.h>
#include <json_object.h>
#include <tgos.h>
#include <tg_utility.h>



//register list
typedef struct
{
    CHAR* path;
    CHAR* keys;
    SharedPreferences_Notification_Callback cb;
    SharedPreferences_WRITE_TYPE type;
}Shared_Preferences_Register_Item;


typedef struct
{
    array_list* list;  //the Shared_Preferences_Register_Item is binded to this arraylist
}Shared_Preferences_Register_List;

static Shared_Preferences_Register_List s_shared_preferences_register_list={0};


//lock&unlock

typedef struct
{
    array_list* lock_list;   //the path is binded to this arraylist
}Shared_Preferences_Lock_List;

static Shared_Preferences_Lock_List s_shared_preferences_lock_list = {0};
static TGOS_HANDLE      s_shared_preferences_sem=0;

static void tg_shared_preferences_lock_free(void* data)
{
    TG_FREE(data);
}

void tg_shared_preferences_manager_init()
{
    if (s_shared_preferences_sem==0)
    {
        //init sem
        //sem_init (&s_shared_preferences_sem, 0, 1);
        s_shared_preferences_sem = tg_os_CreateSemaphore(1);

        //lock&unlock
        memset(&s_shared_preferences_lock_list,0,sizeof(s_shared_preferences_lock_list));

        //register
        memset(&s_shared_preferences_register_list,0,sizeof(s_shared_preferences_register_list));

    }
}


static INT32 tg_shared_preferences_find_lock_path(const CHAR* path)
{
    INT32 i = 0;
    INT32 len = 0;
    struct array_list* list = s_shared_preferences_lock_list.lock_list;
    return_val_if_fail(path,-1);
    return_val_if_fail(list,-1);
    //sem_wait (&s_shared_preferences_sem);
    tg_os_WaitSemaphore(s_shared_preferences_sem);
    len = array_list_length(list);
    for (;i<len;i++)
    {
        CHAR* lock_path =  (CHAR*)array_list_get_idx(list, i);
        if (lock_path==NULL)
            continue;
        else if (strcmp(lock_path,path)==0)
            break;
    }
    //sem_post (&s_shared_preferences_sem);
    tg_os_SignalSemaphore(s_shared_preferences_sem);
    return i<len?i:-1;

}

static INT32 tg_shared_preferences_get_first_free_slot(array_list* list)
{
    INT32 i = 0;
    INT32 len = 0;
    return_val_if_fail(list,-1);

    len = array_list_length(list);
    for (;i<len;i++)
    {
        if (array_list_get_idx(list, i)==NULL)
            break;
    }

    return i;

}

BOOL tg_shared_preferences_is_locked(const CHAR* path)
{
    return (tg_shared_preferences_find_lock_path(path) != -1);

}


BOOL tg_shared_preferences_lock(const CHAR* path,BOOL lock)
{

    INT32 i = 0;
    INT32 len = 0;
    struct array_list* list = NULL;
    INT32 idx = -1;
    //sem_wait (&s_shared_preferences_sem);
    tg_os_WaitSemaphore(s_shared_preferences_sem);
    if (s_shared_preferences_lock_list.lock_list == NULL)
    {
        s_shared_preferences_lock_list.lock_list = array_list_new(tg_shared_preferences_lock_free);
    }
    //sem_post (&s_shared_preferences_sem);
    tg_os_SignalSemaphore(s_shared_preferences_sem);
    list = s_shared_preferences_lock_list.lock_list;
    return_val_if_fail(path,FALSE);
    return_val_if_fail(list,FALSE);
    idx = tg_shared_preferences_find_lock_path(path);
    if (idx>=0)
    {
        if (!lock)
        {
            //sem_wait (&s_shared_preferences_sem);
            tg_os_WaitSemaphore(s_shared_preferences_sem);
            array_list_put_idx(list, idx, NULL);
            //sem_post (&s_shared_preferences_sem);
            tg_os_SignalSemaphore(s_shared_preferences_sem);
        }
        return TRUE;
    }

    else
    {

        if (lock)
        {
            CHAR* lock_path = TG_CALLOC((strlen(path)+1),1);
            strcpy(lock_path,path);
            //sem_wait (&s_shared_preferences_sem);
            tg_os_WaitSemaphore(s_shared_preferences_sem);
            array_list_put_idx(list, tg_shared_preferences_get_first_free_slot(s_shared_preferences_lock_list.lock_list), (void*)lock_path);
            //sem_post (&s_shared_preferences_sem);
            tg_os_SignalSemaphore(s_shared_preferences_sem);
        }

        return TRUE;
    }
}

/*
register for notification, there is a limitation,only first level key can be reigstered
*/

static void tg_shared_preferences_register_free(void* data)
{
    Shared_Preferences_Register_Item* item =(Shared_Preferences_Register_Item*) data;
    TG_FREE(item->keys);
    TG_FREE(item->path);
    TG_FREE(item);
}

// -1: no relationship
// 0: the same path
// 1 keys1 is the parent of keys2
// 2 keys2 is the parent of keys1
typedef enum
{
    Shared_Preferences_Itself,
    Shared_Preferences_Parent,
    Shared_Preferences_Child,
    Shared_Preferences_NO_Relationship,
}Shared_Preferences_Relationship;

static Shared_Preferences_Relationship tg_shared_preferences_is_parent_key(const CHAR* keys1,const CHAR* keys2)
{
    UINT16 path1_len = strlen (keys1);
    UINT16 path2_len = strlen (keys2);
    const CHAR* itor1 = keys1;
    const CHAR* itor2 = keys2;
    INT32 ret = 0;
    if (path1_len>path2_len)
    {
        itor1 = keys2;
        itor2 = keys1;
    }

    while (*itor1 != '\0')
    {
        if (*itor1++ != *itor2++)
            return  Shared_Preferences_NO_Relationship;
    }
    if (path1_len==path2_len)
    {
        return Shared_Preferences_Itself;
    }
    else if (path1_len>path2_len)
    {
        return Shared_Preferences_Child;
    }
    else
    {
        return Shared_Preferences_Parent;
    }
}

static Shared_Preferences_Register_Item* tg_shared_preferences_get_register_item(INT32 idx)
{

    Shared_Preferences_Register_Item* item =NULL;
    //sem_wait (&s_shared_preferences_sem);
    tg_os_WaitSemaphore(s_shared_preferences_sem);
    item =  (Shared_Preferences_Register_Item*)array_list_get_idx(s_shared_preferences_register_list.list, idx);
    //sem_post (&s_shared_preferences_sem);
    tg_os_SignalSemaphore(s_shared_preferences_sem);
    return item;

}
static Shared_Preferences_Register_Item* tg_shared_preferences_find_register_item(const CHAR* path,const CHAR* normalize_key,SharedPreferences_Notification_Callback cb,SharedPreferences_WRITE_TYPE type,INT32* idx)
{
    INT32 i = 0;
    INT32 len = 0;
    Shared_Preferences_Register_Item* item =NULL;
    struct array_list* list = s_shared_preferences_register_list.list;
    return_val_if_fail(path,NULL);
    return_val_if_fail(list,NULL);
    //sem_wait (&s_shared_preferences_sem);
    tg_os_WaitSemaphore(s_shared_preferences_sem);
    len = array_list_length(list);
    for (;i<len;i++)
    {
        item =  (Shared_Preferences_Register_Item*)array_list_get_idx(list, i);
        if (item==NULL || item->path==NULL ||item->keys==NULL)
            continue;
        else if ((item->cb==cb) && (item->type==type) && (strcmp(item->path,path)==0 )&& (strcmp(item->keys,normalize_key)==0 ))
        {
            break;
        }

    }
    //sem_post (&s_shared_preferences_sem);
    tg_os_SignalSemaphore(s_shared_preferences_sem);
    *idx= i;
    return i<len?item:NULL;

}

static BOOL tg_shared_preferences_find_register_items(const CHAR* path,INT32** idx_list,INT32* idx_list_len)
{
    INT32 i = 0;
    INT32 len = 0;
    INT32 count = 0;
    INT32* found_list= NULL;
    Shared_Preferences_Register_Item* item =NULL;
    struct array_list* list = s_shared_preferences_register_list.list;
    return_val_if_fail(path,FALSE);
    return_val_if_fail(list,FALSE);
    //sem_wait (&s_shared_preferences_sem);
    tg_os_WaitSemaphore(s_shared_preferences_sem);
    len = array_list_length(list);
    *idx_list_len=0;
    for (;i<len;i++)
    {
        item =  (Shared_Preferences_Register_Item*)array_list_get_idx(list, i);
        if (item==NULL || item->path==NULL ||item->keys==NULL)
            continue;
        else if (strcmp(item->path,path)==0 )
        {
            if (found_list==NULL)
                found_list=TG_CALLOC_V2(len*sizeof(INT32));
            found_list[count]=i;
            count++;
        }

    }
    //sem_post (&s_shared_preferences_sem);
    tg_os_SignalSemaphore(s_shared_preferences_sem);
    if (found_list)
    {
        *idx_list = found_list;
    }
    *idx_list_len=count;
    return count>0?TRUE:FALSE;

}
#ifdef SharedPreferences_DEBUG
static void tg_shared_preferences_travel_register_list()
{
    INT32 i = 0;
    INT32 len = 0;
    CHAR  str[200];
    Shared_Preferences_Register_Item* item =NULL;
    struct array_list* list = s_shared_preferences_register_list.list;
    //sem_wait (&s_shared_preferences_sem);
    tg_os_WaitSemaphore(s_shared_preferences_sem);
    len = array_list_length(list);
    //output_2_console_1( "travel register list");
    for (;i<len;i++)
    {
        item =  (Shared_Preferences_Register_Item*)array_list_get_idx(list, i);
	 if(item)
	 {
        	sprintf(str,"Path:%s; Key:%s; Type:%d;",item->path,item->keys,item->type);
     //   	output_2_console_1( str);
	 }
    }
    //sem_post (&s_shared_preferences_sem);
    tg_os_SignalSemaphore(s_shared_preferences_sem);


}
#endif
static CHAR* tg_shared_preferences_normalize_keys(const CHAR* keys)
{
    CHAR* normalize_keys;
    CHAR* iter = NULL;
    INT32 len = 0;
    return_val_if_fail(keys,NULL);
    len = strlen(keys);
    normalize_keys = TG_CALLOC(len+1+2 ,1);
    return_val_if_fail(normalize_keys,NULL);
    iter = normalize_keys;
    if (keys[0]!=L'/')
    {
        normalize_keys[0]=L'/';
        iter++;
    }
    strcpy(iter,keys);
    len = strlen(normalize_keys);
    if (normalize_keys[len]==L'/')
    {
        normalize_keys[len]=0;
    }
    return normalize_keys;


}
static BOOL tg_shared_preferences_delete_register_item(INT32 idx)
{
    struct array_list* list = s_shared_preferences_register_list.list;
    return_val_if_fail((idx>=0),FALSE);

    //sem_wait (&s_shared_preferences_sem);
    tg_os_WaitSemaphore(s_shared_preferences_sem);
    array_list_put_idx(list, idx, NULL);
    //sem_post (&s_shared_preferences_sem);
    tg_os_SignalSemaphore(s_shared_preferences_sem);

    return TRUE;
}
static BOOL tg_shared_preferences_add_new_register_item(const CHAR* path,const CHAR* keys,SharedPreferences_Notification_Callback cb,SharedPreferences_WRITE_TYPE type)
{
    struct array_list* list = s_shared_preferences_register_list.list;
    INT32 idx = 0;
    Shared_Preferences_Register_Item* item=NULL;
    BOOL ret = FALSE;
    return_val_if_fail(path,FALSE);
    return_val_if_fail(keys,FALSE);
    //sem_wait (&s_shared_preferences_sem);
    tg_os_WaitSemaphore(s_shared_preferences_sem);
    idx =  tg_shared_preferences_get_first_free_slot(list);
    if (idx >= 0)
    {
        item = TG_CALLOC_V2(sizeof(Shared_Preferences_Register_Item));
        item->cb = cb;
        item->type=type;
        item->path = TG_CALLOC_V2(strlen(path)+1);
        strcpy(item->path,path);
        item->keys= TG_CALLOC_V2(strlen(keys)+1);
        strcpy(item->keys,keys);
        array_list_put_idx(list, idx, item);
        ret = TRUE;
    }
    //sem_post (&s_shared_preferences_sem);
    tg_os_SignalSemaphore(s_shared_preferences_sem);
    return ret;
}
#if 0
static BOOL tg_shared_preferences_update_register_item(INT32 idx,const CHAR* path,const CHAR* keys,SharedPreferences_Notification_Callback cb,SharedPreferences_WRITE_TYPE type)
{
    struct array_list* list = s_shared_preferences_register_list.list;

    Shared_Preferences_Register_Item* item = TG_CALLOC_V2(sizeof(Shared_Preferences_Register_Item));
    return_val_if_fail(path,FALSE);
    return_val_if_fail(keys,FALSE);
    item->cb = cb;
    item->type=type;
    item->path = TG_CALLOC_V2(strlen(path)+1);
    strcpy(item->path,path);
    item->keys= TG_CALLOC_V2(strlen(keys)+1);
    strcpy(item->keys,keys);
    sem_wait (&s_shared_preferences_sem);
    array_list_put_idx(list, idx, item);
    sem_post (&s_shared_preferences_sem);


    return TRUE;
}
#endif
/*
old key path: key1_path, new key path: key2_path

if key1_path == key2_path, if either type or cb is different , add a new item. or do noting
if key1_path is the parent of key2_path, if either type or cb is different , add a new item. or do nothing.
if key1_path is the child of key2 path, a. either type or cb is different, add a new item or replace the item with key1_path using item with key2_path

==>

1. either type or cb is different, add a new item
2. if both type anc cb are the same one, check key1_path and key2_path, if key1_path is the child of key2_path, replace key1_path using key2_path

*/
INT32 tg_shared_preferences_register(const CHAR* path,const CHAR* keys,SharedPreferences_Notification_Callback cb,SharedPreferences_WRITE_TYPE type)
{


    INT32* idx_list = NULL;
    INT32 idx=0;
    CHAR* normalize_key = NULL;
    Shared_Preferences_Register_Item* item = NULL;
    INT32 ret = SharedPreferences_SUCC;//SharedPreferences_ERROR;
    //sem_wait (&s_shared_preferences_sem);
    tg_os_WaitSemaphore(s_shared_preferences_sem);
    if (s_shared_preferences_register_list.list == NULL)
    {
        s_shared_preferences_register_list.list = array_list_new(tg_shared_preferences_register_free);
    }
    //sem_post (&s_shared_preferences_sem);
    tg_os_SignalSemaphore(s_shared_preferences_sem);

    return_val_if_fail(path,SharedPreferences_PATH_ERROR);

    normalize_key = tg_shared_preferences_normalize_keys(keys);
    return_val_if_fail(normalize_key,SharedPreferences_ERROR);
    item = tg_shared_preferences_find_register_item(path,normalize_key,cb,type,&idx);
    
    if (!item)   //found at least one item
    {
        tg_shared_preferences_add_new_register_item(path,normalize_key,cb,type);

    }
    TG_FREE(normalize_key);
#ifdef SharedPreferences_DEBUG
     tg_shared_preferences_travel_register_list();
#endif
    return ret;
}


INT32 tg_shared_preferences_unregister(const CHAR* path,const CHAR* keys,SharedPreferences_Notification_Callback cb,SharedPreferences_WRITE_TYPE type)
{

    INT32 i = 0;
    INT32 len = 0;
    INT32 idx = 0;
    struct array_list* list = NULL;
    CHAR* normalize_key = NULL;
    Shared_Preferences_Register_Item* item = NULL;

    Shared_Preferences_Relationship relationship;
    list = s_shared_preferences_register_list.list;
    return_val_if_fail(path,SharedPreferences_PATH_ERROR);
    return_val_if_fail(list,SharedPreferences_ERROR);
    normalize_key = tg_shared_preferences_normalize_keys(keys);
    return_val_if_fail(normalize_key,SharedPreferences_ERROR);

    item = tg_shared_preferences_find_register_item(path,normalize_key,cb,type,&idx );
    TG_FREE(normalize_key);
    return_val_if_fail(item,SharedPreferences_ERROR);


    relationship = tg_shared_preferences_is_parent_key(item->keys,normalize_key);
    //  if (relationship==Shared_Preferences_Itself||relationship==Shared_Preferences_Parent)
    tg_shared_preferences_delete_register_item(idx);

    return SharedPreferences_SUCC;
}
INT32 tg_shared_preferences_unregister_all()
{
    INT32 i = 0;
    INT32 len = 0;
    struct array_list* list = s_shared_preferences_register_list.list;
    //sem_wait (&s_shared_preferences_sem);
    tg_os_WaitSemaphore(s_shared_preferences_sem);
    len = array_list_length(list);
    for(;i<len;i++)
    {
    	array_list_put_idx(list, i, NULL);
    }
    //sem_post (&s_shared_preferences_sem);
    tg_os_SignalSemaphore(s_shared_preferences_sem);

    return SharedPreferences_SUCC;
}
/*
Ò». delete a key
1. if parent is registered:
   a. parent register a delete mode,  do nothing
   b.parent register a  add mode,  do nothing
   c.parent register a  modify mode, do callback

2. if child is registered:
   a. child register a delete mode,  do callback
   b.child register a  add mode,  do nothing
   c.child register a  modify mode, do nothing

3. if itself is registered:
   a. register a delete mode,  do callback
   b. register a  add mode,  do nothing
   c. register a  modify mode, do nothing

¶þ. add a key
1. if parent is registered:
   a. parent register a delete mode,  do nothing
   b.parent register a  add mode,  do nothing
   c.parent register a  modify mode, do callback

2. if child is registered:
   a. child register a delete mode,  do  nothing
   b.child register a  add mode,  do nothing
   c.child register a  modify mode, do nothing

3. if itself is registered:
   a. register a delete mode,  do nothing
   b. register a  add mode,  do callback
   c. register a  modify mode, do nothing

Èý. modify a key
1. if parent is registered:
   a. parent register a delete mode,  do nothing
   b.parent register a  add mode,  do  nothing
   c.parent register a  modify mode, do callback

2. if child is registered:
   a. child register a delete mode,  do  nothing
   b.child register a  add mode,  do nothing
   c.child register a  modify mode, do nothing

3. if itself is registered:
   a. register a delete mode,  do  nothing
   b. register a  add mode,  do nothing
   c. register a  modify mode, do  callback

*/
#if 0
BOOL tg_shared_preferences_check_register_item(const CHAR* path,const CHAR* key_path,SharedPreferences_WRITE_TYPE type)
{
    CHAR* normalize_key = NULL;
    Shared_Preferences_Register_Item* item = NULL;
    INT32 idx = 0;
    BOOL ret= FALSE;
    return_val_if_fail(path,FALSE);
    return_val_if_fail(key_path,FALSE);
    normalize_key = tg_shared_preferences_normalize_keys(key_path);
    return_val_if_fail(normalize_key,FALSE);
//    item = tg_shared_preferences_find_register_path(path,&idx );
    if (tg_shared_preferences_find_register_items( path,&idx_list,&len))   //found at least one item
        if (item)
        {
            Shared_Preferences_Relationship relationship = tg_shared_preferences_is_parent_key(item->keys,normalize_key);
            if ((relationship==Shared_Preferences_Itself && item->type==type) || (relationship==Shared_Preferences_Parent &&type==SharedPreferences_Modify_Mode))  //complete match
            {
                item->cb();
                ret= TRUE;
            }
            else if (SharedPreferences_Delete_Mode == type )
            {
                if (  (relationship==Shared_Preferences_Child &&type==SharedPreferences_Delete_Mode) )
                {
                    item->cb();
                    ret= TRUE;
                }
            }

        }
    TG_FREE(normalize_key);


    return ret;

}
#else
BOOL tg_shared_preferences_check_register_item(const CHAR* path,const CHAR* key_path,SharedPreferences_WRITE_TYPE type)
{


    INT32 len = 0;
    INT32* idx_list = NULL;
    INT32 idx=0;
    CHAR* normalize_key = NULL;
    Shared_Preferences_Register_Item* item = NULL;
    INT32 ret = SharedPreferences_SUCC;//SharedPreferences_ERROR;
    return_val_if_fail(path,FALSE);
    return_val_if_fail(key_path,FALSE);
    normalize_key = tg_shared_preferences_normalize_keys(key_path);
    return_val_if_fail(normalize_key,FALSE);

    if (tg_shared_preferences_find_register_items( path,&idx_list,&len))   //found at least one item
    {
        INT32 i=0;
        for (;i<len;i++)
        {
            
            item = tg_shared_preferences_get_register_item(idx_list[i]);
            if (item)
            {
                Shared_Preferences_Relationship relationship = tg_shared_preferences_is_parent_key(item->keys,normalize_key);
                if ((relationship==Shared_Preferences_Itself && item->type==type) || (relationship==Shared_Preferences_Parent &&item->type==SharedPreferences_Modify_Mode))  //complete match
                {
                   ret= TRUE;
                   if( item->cb())
				break;
                }
                else if (SharedPreferences_Delete_Mode == type )
                {
                    if (  (relationship==Shared_Preferences_Child &&type==SharedPreferences_Delete_Mode) )
                    {
                    	   ret= TRUE;
                        if( item->cb())
				break;
                        
                    }
                }

            }
            
        }

    }
    TG_FREE(idx_list);
    TG_FREE(normalize_key);
    return ret;
}
#endif