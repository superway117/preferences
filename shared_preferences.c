
#include <common.h>
#include <tgfile.h>
#include <json_object.h>
#include <printbuf.h>
#include <bits.h>
#include <linkhash.h>
#include <json_tokener.h>
#include <arraylist.h>
#include "shared_preferences.h"
#include <tg_utility.h>
#include <stdio.h>

struct _SharedPreferences
{
    struct json_object* obj;
    CHAR * path;
    SharedPreferences_Open_Mode mode;
//	void* priv_subclass;
};

extern BOOL tg_shared_preferences_is_locked(const CHAR* path);
extern BOOL tg_shared_preferences_lock(const CHAR* path,BOOL lock);
extern BOOL tg_shared_preferences_check_register_item(const CHAR* path,const CHAR* key_path,SharedPreferences_WRITE_TYPE type);

#define SHARED_PREFERENCE_FILE_BUF_SIZE 4096


BOOL tg_shared_preferences_is_ready(const CHAR* path)
{
    TG_FSSTAT stat;
    return (0 ==  tg_stat(path, &stat));
}

SharedPreferences* tg_shared_preferences_open(const CHAR* path,SharedPreferences_Open_Mode mode)
{

    struct json_object *obj;

    UINT32 ret;
    TG_FILE * fp = NULL;
    SharedPreferences* sp = NULL;
    BOOL new_file = FALSE;
    return_val_if_fail(path,NULL);
    if (mode == SharedPreferences_ReadWrite_Mode)  //if already lock, can not open it with write mode
    {
        if (tg_shared_preferences_is_locked(path))
            return NULL;
        if (!tg_shared_preferences_is_ready(path))
        {
            fp =tg_fopen(path, "w+");
	     return_val_if_fail(fp,NULL);
            tg_fclose(fp);
            new_file = TRUE;
        }
    }
    if (!new_file)
    {
        struct printbuf *pb;
        CHAR* buf=NULL;
        fp =tg_fopen(path, "r");
        return_val_if_fail((fp),NULL);

        if (!(pb = printbuf_new()))
        {
            tg_fclose(fp);
            return NULL;
        }
        buf =TG_MALLOC(SHARED_PREFERENCE_FILE_BUF_SIZE);
        while ((ret = tg_fread(buf, SHARED_PREFERENCE_FILE_BUF_SIZE,1,fp)) > 0)
        {
            printbuf_memappend(pb, buf, ret);
        }
        TG_FREE(buf);
        tg_fclose(fp);

        obj = json_tokener_parse(pb->buf);
        printbuf_free(pb);
    }
    else
    {
        obj = json_object_new_object();
    }
    sp = TG_CALLOC_V2(sizeof(SharedPreferences));
    return_val_if_fail((sp),NULL);
    sp->obj=obj;
    sp->mode = mode;
    if (mode == SharedPreferences_ReadWrite_Mode)
    {
        sp->path = TG_CALLOC_V2(strlen(path)+1);
        strcpy(sp->path ,path);
        tg_shared_preferences_lock(path,TRUE);

    }
    return sp;
}



INT32 tg_shared_preferences_close(SharedPreferences* thiz)
{

    json_object_put(thiz->obj);
    if (thiz->mode == SharedPreferences_ReadWrite_Mode && thiz->path!=NULL)
    {
        tg_shared_preferences_lock(thiz->path,FALSE);

    }
    TG_FREE(thiz->path);
    TG_FREE(thiz);
    return SharedPreferences_SUCC;
}

static INT32 tg_shared_preferences_write_json_str(const CHAR* path,const CHAR* json_str)
{

    UINT32 ret;
    UINT32 wpos, wsize;
    TG_FILE * fp = NULL;
    if (!json_str|| !path )
    {
        return SharedPreferences_ERROR;
    }
    fp =tg_fopen(path, "w+");
    return_val_if_fail(fp,SharedPreferences_OPEN_FILE_ERROR);

    wsize = (UINT32)(strlen(json_str)); /* CAW: probably unnecessary, but the most 64bit safe */
    wpos = 0;
    while (wpos < wsize)
    {
        if ((ret = tg_fwrite( json_str + wpos, wsize-wpos,1,fp)) ==0 )
        {
            break;
        }

        /* because of the above check for ret < 0, we can safely cast and add */
        wpos += (UINT32)ret;
    }

    tg_fclose(fp);
    return SharedPreferences_SUCC;
}


INT32 tg_shared_preferences_commit(SharedPreferences* thiz)
{
    const CHAR *json_str;
    INT32 ret =SharedPreferences_SUCC ;

    if (!thiz->obj|| !thiz->path || thiz->mode == SharedPreferences_Read_Mode)
    {
        return SharedPreferences_ERROR;
    }

    if (!(json_str = json_object_to_json_string(thiz->obj))) {
        return SharedPreferences_ERROR;
    }

    return ( tg_shared_preferences_write_json_str(thiz->path,json_str));

}


INT32 tg_shared_preferences_delete(const CHAR* path)
{
    return (tg_remove(path)==0)?SharedPreferences_SUCC:SharedPreferences_ERROR;
}




INT32 tg_shared_preferences_init_from_string(const CHAR* path,const CHAR* json_str)
{
    if (path == NULL  || json_str==NULL)
        return SharedPreferences_ERROR;
    return tg_shared_preferences_write_json_str(path,json_str);
}

INT32 tg_shared_preferences_init_from_file(const CHAR* path,const CHAR* default_path)
{
    TG_FSSTAT stat;
    TG_FILE * src_fp = NULL;
    TG_FILE * dest_fp = NULL;
    UINT32 ret;

    CHAR* buf=NULL;
    if (path == NULL  || default_path==NULL)
        return SharedPreferences_ERROR;
    if (0 !=  tg_stat(default_path, &stat))
        return SharedPreferences_PATH_ERROR;

    dest_fp =tg_fopen(path, "w+");
    return_val_if_fail(dest_fp,SharedPreferences_OPEN_FILE_ERROR);
    src_fp = tg_fopen(default_path, "r");
    buf =TG_MALLOC(SHARED_PREFERENCE_FILE_BUF_SIZE);
    while (1)
    {
        if ((ret = tg_fread(buf, SHARED_PREFERENCE_FILE_BUF_SIZE,1,src_fp)) == 0)  break;
        if ((ret = tg_fwrite( buf, ret,1,dest_fp)) ==0 )   break;
    }
    TG_FREE(buf);
    tg_fclose(src_fp);
    tg_fclose(dest_fp);
    return 0;
}


/*
key path parser

*/

static BOOL tg_shared_preferences_parse_keypath(const CHAR* key_path,struct array_list* list,INT32* key_num)
{
    UINT16 path_len = 0 ;
    CHAR* key=NULL;
    const CHAR* itor1 = key_path;
    const CHAR* itor2 = key_path;
    INT32 counter = 0;
    return_val_if_fail(key_path,FALSE);

    path_len = strlen(key_path);
    return_val_if_fail((path_len>0),FALSE);
    if (key_path[0]== '/')
    {

        itor1++;
        itor2++;

    }
    while (1)
    {
        if (*itor1 == '\0')
        {

            INT32 len = (itor1-itor2);
            if (len==0) break;
            key =TG_CALLOC(len+1,1);
            strncpy(key,itor2,len);
            array_list_add(list, (void *)key);
            counter++;
            break;
        }
        else if (*itor1 == '/')
        {
            INT32 len = (itor1-itor2);
            if (len==0) break;
            key =TG_CALLOC(len+1,1);
            strncpy(key,itor2,len);
            array_list_add(list, (void *)key);
            counter++;
            itor2=itor1+1;
        }
        itor1++;
    }
    *key_num = counter;
    return counter;

}

static void tg_shared_preferences_key_free(void* data)
{
    TG_FREE(data);
}

/*
first level  interface
*/
/*
require interface
*/
json_object * tg_shared_preferences_find_leaf_obj(SharedPreferences* thiz,const CHAR* key_path)
{
    struct json_object *jso=NULL;
    struct array_list* key_list = NULL;
    INT32 key_list_len = 0;
    INT32 idx = 0;

    return_val_if_fail((thiz&&key_path),NULL);
    if (strcmp(key_path,"/")==0)
        return thiz->obj;
    key_list = array_list_new(tg_shared_preferences_key_free);
    return_val_if_fail((key_list),NULL);
    return_val_if_fail(tg_shared_preferences_parse_keypath(key_path,key_list,&key_list_len),NULL);
    for (jso=thiz->obj;idx<key_list_len &&jso;idx++)
    {
        jso = json_object_object_get(jso,(CHAR*)array_list_get_idx(key_list,idx));

    }
    array_list_free(key_list);
    return jso;

}

json_object * tg_shared_preferences_find_parent_of_leaf(SharedPreferences* thiz,const CHAR* key_path,CHAR** leaf_key)
{
    struct json_object *jso=NULL;
    struct array_list* key_list = NULL;
    INT32 key_list_len = 0;
    INT32 idx = 0;
    return_val_if_fail((thiz&&key_path),NULL);
    key_list = array_list_new(tg_shared_preferences_key_free);
    return_val_if_fail((key_list),NULL);
    return_val_if_fail(tg_shared_preferences_parse_keypath(key_path,key_list,&key_list_len),NULL);
    for (jso=thiz->obj;idx<key_list_len-1;idx++)
    {
        jso = json_object_object_get(jso,(CHAR*)array_list_get_idx(key_list,idx));
        if (jso==NULL)
            break;

    }
    if (jso!=NULL)
    {
        CHAR* key = (CHAR*)array_list_get_idx(key_list,key_list_len-1);
        //ASSERT(key);
        *leaf_key = TG_CALLOC((strlen(key)+1),1);
        strcpy(*leaf_key,key);
    }
    array_list_free(key_list);
    return jso;

}
//find parent of leaf, if the path is not completely in SharedPreferences, append the lh obj
json_object * tg_shared_preferences_check_parent_of_leaf(SharedPreferences* thiz,const CHAR* key_path,CHAR** leaf_key)
{
    struct json_object *jso=NULL;
    struct json_object *next_jso=NULL;
    struct array_list* key_list = NULL;
    INT32 key_list_len = 0;
    INT32 idx = 0;
    return_val_if_fail((thiz&&key_path),NULL);
    key_list = array_list_new(tg_shared_preferences_key_free);
    return_val_if_fail((key_list),NULL);
    return_val_if_fail(tg_shared_preferences_parse_keypath(key_path,key_list,&key_list_len),NULL);
    for (jso=thiz->obj;idx<key_list_len-1;idx++)
    {
        next_jso = json_object_object_get(jso,(CHAR*)array_list_get_idx(key_list,idx));
        if (next_jso==NULL)
        {
            do{
                CHAR*key = (CHAR*)array_list_get_idx(key_list,idx);
                next_jso=json_object_new_object();
                json_object_object_add(jso,key,next_jso);
                idx++;
                jso = next_jso;
            }while (idx<key_list_len-1);

            break;
        }
        else
            jso = next_jso;

    }
    if (jso!=NULL)
    {
        CHAR* key = (CHAR*)array_list_get_idx(key_list,key_list_len-1);
        //ASSERT(key);
        *leaf_key = TG_CALLOC((strlen(key)+1),1);
        strcpy(*leaf_key,key);
    }
    array_list_free(key_list);
    return jso;

}


static CHAR tg_shared_preferences_char_2_digit(const CHAR data)
{
    CHAR offset = 0;
    if (data<='9' && data>='0')
    {
        offset = '1'-0x1;
    }
    else if (data<='f' && data>='a')
    {
        offset = 'a'-0xa;
    }
    else if (data<='F' && data>='A')
    {
        offset = 'A'-0xa;
    }
//add a asset here to do
    return (CHAR)((data-offset)&0xFF);

}


static void tg_shared_preferences_char_2_hex(const CHAR* data,CHAR* ret)
{
    CHAR high = 0,low = 0;

    high = tg_shared_preferences_char_2_digit(data[0]);
    low = tg_shared_preferences_char_2_digit(data[1]);
    *ret= (CHAR)((high<<4) | low);

}
static void* tg_shared_preferences_string_2_serialization(const CHAR* serial_str)
{
    INT32 len = strlen(serial_str);
    INT32 i = 0;
    void* serial_obj = NULL;
    CHAR* serial_obj_p = NULL;
    if ((len%2)!=0)
        return NULL;
    serial_obj  = TG_CALLOC_V2((len/2)+2);  //"41"->0x41, two bytes convert to one hex byte
    serial_obj_p= serial_obj;
    for (;i<len;i+=2)   //step is two bytes
    {

        tg_shared_preferences_char_2_hex(serial_str+i,serial_obj_p++);
    }
    return serial_obj;

}

INT32 tg_shared_preferences_get_serialization(SharedPreferences* thiz,const CHAR* key_path,void** ret)
{

    CHAR* ori_str=NULL;
    struct json_object *jso = tg_shared_preferences_find_leaf_obj(thiz,key_path);
    return_val_if_fail((jso),SharedPreferences_KEY_NOT_EXIST);
    ori_str =  (CHAR*)json_object_get_string(jso);
    return_val_if_fail((ori_str),SharedPreferences_ERROR);
    *ret=tg_shared_preferences_string_2_serialization(ori_str);
    return ( *ret!=NULL)?SharedPreferences_SUCC:SharedPreferences_ERROR;

}
INT32 tg_shared_preferences_get_string_16(SharedPreferences* thiz,const CHAR* key_path,WCHAR** ret)
{

    return tg_shared_preferences_get_serialization(thiz,key_path,(void**)ret);

}

INT32 tg_shared_preferences_get_string(SharedPreferences* thiz,const CHAR* key_path,CHAR** ret)
{
    CHAR* ori_str=NULL;
    struct json_object *jso = tg_shared_preferences_find_leaf_obj(thiz,key_path);
    return_val_if_fail((jso),SharedPreferences_KEY_NOT_EXIST);
    ori_str= ( CHAR*)json_object_get_string(jso);
    return_val_if_fail((ori_str),SharedPreferences_ERROR);
    *ret = TG_CALLOC_V2(strlen(ori_str)+1);
    strcpy(*ret,ori_str);
    return SharedPreferences_SUCC;
}

const CHAR* tg_shared_preferences_get_string_v2(SharedPreferences* thiz,const CHAR* key_path)
{
    struct json_object *jso = tg_shared_preferences_find_leaf_obj(thiz,key_path);
    return_val_if_fail((jso),NULL);
    return ( const CHAR*)json_object_get_string(jso);

}

INT32 tg_shared_preferences_get_int(SharedPreferences* thiz,const CHAR* key_path,INT32* ret)
{
    struct json_object *jso = tg_shared_preferences_find_leaf_obj(thiz,key_path);
    return_val_if_fail((jso),SharedPreferences_KEY_NOT_EXIST);
    *ret =  (INT32)json_object_get_int(jso);
    return SharedPreferences_SUCC;

}

INT32 tg_shared_preferences_get_double(SharedPreferences* thiz,const CHAR* key_path,double* ret)
{
    struct json_object *jso = tg_shared_preferences_find_leaf_obj(thiz,key_path);
    return_val_if_fail((jso),SharedPreferences_KEY_NOT_EXIST);
    *ret =  (double)json_object_get_double(jso);
    return SharedPreferences_SUCC;

}

INT32 tg_shared_preferences_get_bool(SharedPreferences* thiz,const CHAR* key_path,BOOL* ret)
{
    struct json_object *jso = tg_shared_preferences_find_leaf_obj(thiz,key_path);
    return_val_if_fail((jso),SharedPreferences_KEY_NOT_EXIST);
    *ret =  (double)json_object_get_boolean(jso);
    return SharedPreferences_SUCC;

}


INT32 tg_shared_preferences_get_child_count(SharedPreferences* thiz,const CHAR* key_path,INT32* count)
{
    struct json_object *jso = tg_shared_preferences_find_leaf_obj(thiz,key_path);
    struct lh_table* lh = NULL;
    return_val_if_fail((jso),SharedPreferences_KEY_NOT_EXIST);
    lh =  json_object_get_object(jso);
    if (!lh)
        *count = 0;
    else
        *count = lh->count;
    return SharedPreferences_SUCC;

}

INT32 tg_shared_preferences_produce_child_path_by_idx(SharedPreferences* thiz,const CHAR* key_path,INT32 index,CHAR** ret)
{
    struct json_object *jso = tg_shared_preferences_find_leaf_obj(thiz,key_path);
    struct lh_table* lh = NULL;
    INT32 len = 0;
    CHAR* path = NULL;
    CHAR* key = NULL;
    return_val_if_fail((jso),SharedPreferences_KEY_NOT_EXIST);
    lh =  json_object_get_object(jso);
    if (!lh || lh->count<=index)
        return SharedPreferences_KEY_NOT_EXIST;
    key=(CHAR*)lh_table_get_key_by_idx(lh,index);
    if (!key)
        return SharedPreferences_KEY_NOT_EXIST;
    path = TG_CALLOC_V2(strlen(key_path)+strlen(key)+2);
    len =strlen(key_path);
    if (key_path[len-1]=='/')
        sprintf(path,"%s%s",key_path,key);
    else
        sprintf(path,"%s/%s",key_path,key);
    *ret=path;
    return SharedPreferences_SUCC;

}

/*
delete interface
*/
INT32 tg_shared_preferences_delete_key(SharedPreferences* thiz,const CHAR* key_path)
{
    CHAR* leaf_key = NULL;
    struct json_object *jso = tg_shared_preferences_find_parent_of_leaf(thiz,key_path,&leaf_key);
    INT32 ret = SharedPreferences_SUCC;

    return_val_if_fail((jso),SharedPreferences_KEY_NOT_EXIST);
    if (json_object_object_del_v2(jso,leaf_key)==0)
    {

        tg_shared_preferences_check_register_item(thiz->path,key_path,SharedPreferences_Delete_Mode);

    }
    else
        ret = SharedPreferences_KEY_NOT_EXIST;
    TG_FREE(leaf_key);
    return SharedPreferences_SUCC;

}

/*
put interface
*/

static CHAR* tg_shared_preferences_produce_serialization_string(const void* value,INT32 len)
{
    CHAR* value_char = NULL;
    INT32 i = 0;
    UINT8* value_int =(UINT8*)value;
    CHAR tmp_str[8];
    value_char=TG_CALLOC_V2(len*2+2); //one byte needs two bytes to store the hex, e.g. 0x41->"41"
    for (i=0;i<len;i++)
    {
        if (value_int[i]>=0 && value_int[i] <= 15)  //0-15 use one byte
            sprintf(tmp_str,"0%x",value_int[i]);
        else
            sprintf(tmp_str,"%x",value_int[i]);
        strcat(value_char,tmp_str);
    }
    return value_char;

}


INT32 tg_shared_preferences_put_serialization(SharedPreferences* thiz,const CHAR* key_path,const void* value,INT32 len)
{
    CHAR* leaf_key = NULL;
    CHAR* value_char = NULL;
    struct json_object *jso = NULL;
    struct json_object* string_obj = NULL;
    INT32 is_new=1;

    SharedPreferences_WRITE_TYPE type = SharedPreferences_Modify_Mode;
    return_val_if_fail((value),SharedPreferences_ERROR);
    return_val_if_fail((thiz->mode==SharedPreferences_ReadWrite_Mode),SharedPreferences_ERROR);
    jso = tg_shared_preferences_check_parent_of_leaf(thiz,key_path,&leaf_key);
    return_val_if_fail((jso),SharedPreferences_KEY_NOT_EXIST);

    value_char=tg_shared_preferences_produce_serialization_string(value,len);

    string_obj = json_object_new_string((const CHAR*)value_char);
    is_new=json_object_object_add_v2(jso,leaf_key,string_obj);
    TG_FREE(value_char);
    if (is_new)
        type = SharedPreferences_Add_Mode;

    tg_shared_preferences_check_register_item(thiz->path,key_path,type);
    TG_FREE(leaf_key);
    return SharedPreferences_SUCC;

}

INT32 tg_shared_preferences_put_string_16(SharedPreferences* thiz,const CHAR* key_path,const WCHAR* value)
{
    INT32 len = 0;
    return_val_if_fail((value),SharedPreferences_ERROR);
    len= wstrlen(value)*2+2;
    return tg_shared_preferences_put_serialization(thiz,key_path,value,len);

}

INT32 tg_shared_preferences_put_string(SharedPreferences* thiz,const CHAR* key_path,const CHAR* value)
{
    CHAR* leaf_key = NULL;
    struct json_object *jso = NULL;
    struct json_object* string_obj = NULL;
    INT32 is_new=1;
    SharedPreferences_WRITE_TYPE type = SharedPreferences_Modify_Mode;
    return_val_if_fail((value),SharedPreferences_ERROR);
    return_val_if_fail((thiz->mode==SharedPreferences_ReadWrite_Mode),SharedPreferences_ERROR);
    jso = tg_shared_preferences_check_parent_of_leaf(thiz,key_path,&leaf_key);
    return_val_if_fail((jso),SharedPreferences_KEY_NOT_EXIST);

    string_obj = json_object_new_string((const CHAR*)value);
    is_new=json_object_object_add_v2(jso,leaf_key,string_obj);
    if (is_new)
    {
        type = SharedPreferences_Add_Mode;
    }
    tg_shared_preferences_check_register_item(thiz->path,key_path,type);
    TG_FREE(leaf_key);
    return SharedPreferences_SUCC;

}
INT32 tg_shared_preferences_put_int(SharedPreferences* thiz,const CHAR* key_path,INT32 value)
{
    CHAR* leaf_key = NULL;
    struct json_object *jso = NULL;
    struct json_object* obj = NULL;
    INT32 is_new=1;
    SharedPreferences_WRITE_TYPE type = SharedPreferences_Modify_Mode;
    return_val_if_fail((thiz->mode==SharedPreferences_ReadWrite_Mode),SharedPreferences_ERROR);
    jso = tg_shared_preferences_check_parent_of_leaf(thiz,key_path,&leaf_key);
    return_val_if_fail((jso),SharedPreferences_KEY_NOT_EXIST);

    obj = json_object_new_int(value);
    is_new=json_object_object_add_v2(jso,leaf_key,obj);
    if (is_new)
    {
        type = SharedPreferences_Add_Mode;
    }
    tg_shared_preferences_check_register_item(thiz->path,key_path,type);
    TG_FREE(leaf_key);
    return SharedPreferences_SUCC;

}

INT32 tg_shared_preferences_put_double(SharedPreferences* thiz,const CHAR* key_path,double value)
{
    CHAR* leaf_key = NULL;
    struct json_object *jso = NULL;
    struct json_object* obj = NULL;
    INT32 is_new=1;
    SharedPreferences_WRITE_TYPE type = SharedPreferences_Modify_Mode;
    return_val_if_fail((thiz->mode==SharedPreferences_ReadWrite_Mode),SharedPreferences_ERROR);
    jso = tg_shared_preferences_check_parent_of_leaf(thiz,key_path,&leaf_key);
    return_val_if_fail((jso),SharedPreferences_KEY_NOT_EXIST);

    obj = json_object_new_double(value);
    is_new=json_object_object_add_v2(jso,leaf_key,obj);
    if (is_new)
    {
        type = SharedPreferences_Add_Mode;
    }
    tg_shared_preferences_check_register_item(thiz->path,key_path,type);
    TG_FREE(leaf_key);
    return SharedPreferences_SUCC;

}

INT32 tg_shared_preferences_put_bool(SharedPreferences* thiz,const CHAR* key_path,BOOL value)
{
    CHAR* leaf_key = NULL;
    struct json_object *jso = NULL;
    struct json_object* obj = NULL;
    INT32 is_new=1;
    SharedPreferences_WRITE_TYPE type = SharedPreferences_Modify_Mode;
    return_val_if_fail((thiz->mode==SharedPreferences_ReadWrite_Mode),SharedPreferences_ERROR);
    jso = tg_shared_preferences_check_parent_of_leaf(thiz,key_path,&leaf_key);
    return_val_if_fail((jso),SharedPreferences_KEY_NOT_EXIST);

    obj = json_object_new_boolean(value);
    is_new=  json_object_object_add_v2(jso,leaf_key,obj);
    if (is_new)
    {
        type = SharedPreferences_Add_Mode;
    }
    tg_shared_preferences_check_register_item(thiz->path,key_path,type);
    TG_FREE(leaf_key);
    return SharedPreferences_SUCC;

}
