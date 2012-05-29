
#ifndef __TG_SHARED_PREFERENCES_H__
#define __TG_SHARED_PREFERENCES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <common.h>

struct _SharedPreferences;

typedef struct _SharedPreferences  SharedPreferences;


typedef enum
{
	SharedPreferences_SUCC,
	SharedPreferences_ERROR=-1,
	SharedPreferences_KEY_NOT_EXIST=-2,
	SharedPreferences_OCCUPIED=-3,
	SharedPreferences_PATH_ERROR=-4,
	SharedPreferences_OPEN_FILE_ERROR=-5,
	SharedPreferences_LACK_MEMORY=-6,
}SharedPreferences_ERROR_CODE;


typedef enum
{
	SharedPreferences_Read_Mode,
	SharedPreferences_ReadWrite_Mode,
}SharedPreferences_Open_Mode;

typedef enum
{
	SharedPreferences_Delete_Mode,
	SharedPreferences_Add_Mode,
	SharedPreferences_Modify_Mode,
}SharedPreferences_WRITE_TYPE;


typedef INT32 (*SharedPreferences_Notification_Callback ) (  );
 


/**
   initilizate preferences manager, it only needs to be called when power on
 */
extern void tg_shared_preferences_manager_init();    

/**
   open one preference
   @param [in] path SharedPreferences path
   @param [in] mode open mode
   @return SharedPreferences handle
 */
extern SharedPreferences* tg_shared_preferences_open(const CHAR* path,SharedPreferences_Open_Mode mode);

/**
   close one preference
   @param [in] thiz SharedPreferences handle
   @return error code
 */
extern INT32 tg_shared_preferences_close(SharedPreferences* thiz);


/**
   commit the modify on one preference,after modify or delete, need to commit to save the changes
   @param [in] thiz SharedPreferences handle
   @return error code
 */
extern INT32 tg_shared_preferences_commit(SharedPreferences* thiz);


/**
   delete sharepreference file
   @param [in] path SharedPreferences path
   @return error code
 */
extern INT32 tg_shared_preferences_delete(const CHAR* path);

/**
   check whether or not  the sharepreference file is exist
   @param [in] path SharedPreferences path
   @return error code
 */
extern BOOL tg_shared_preferences_is_ready(const CHAR* path);

/**
   create/reset a sharepreference file using json string
   @param [in] path SharedPreferences path
   @param [in] json_str json string
   @return error code
 */
extern INT32 tg_shared_preferences_init_from_string(const CHAR* path,const CHAR* json_str);

/**
   create/reset a sharepreference file using default file
   @param [in] path SharedPreferences path
   @param [in] default_path json file path
   @return error code
 */
extern INT32 tg_shared_preferences_init_from_file(const CHAR* path,const CHAR* default_path);

/**
   register for notification, there is a limitation,only first level key can be reigstered
   @param [in] path SharedPreferences path
   @param [in] keys item path
   @param [in] cb notify callback
   @param [in] type type
   @return error code
 */
extern INT32 tg_shared_preferences_register(const CHAR* path,const CHAR* keys,SharedPreferences_Notification_Callback cb,SharedPreferences_WRITE_TYPE type);


/**
   unregister one data notification
   @param [in] path SharedPreferences path
   @param [in] keys item path
   @param [in] cb notify callback
   @param [in] type type
   @return error code
 */
extern INT32 tg_shared_preferences_unregister(const CHAR* path,const CHAR* keys,SharedPreferences_Notification_Callback cb,SharedPreferences_WRITE_TYPE type);


/**
   unregister all data notification
   @return error code
 */
extern INT32 tg_shared_preferences_unregister_all();


/**
   Retrieve a wstring value from the preferences.
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @return wstring
 */
extern INT32 tg_shared_preferences_get_serialization(SharedPreferences* thiz,const CHAR* key_path,void** ret);

/**
   Retrieve a wstring value from the preferences.
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @param [out] ret  save wstring value
   @return error code
 */
extern INT32 tg_shared_preferences_get_string_16(SharedPreferences* thiz,const CHAR* key_path,WCHAR** ret);

/**
   Retrieve a string value from the preferences.
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @param [out] ret  save string value
   @return error code
 */
extern INT32 tg_shared_preferences_get_string(SharedPreferences* thiz,const CHAR* key_path,CHAR** ret);

/**
   Retrieve a string value from the preferences.
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @return string value
 */
extern const CHAR* tg_shared_preferences_get_string_v2(SharedPreferences* thiz,const CHAR* key_path);

/**
   Retrieve a int value from the preferences.
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @param [out] ret  save int value
   @return error code
 */
extern INT32 tg_shared_preferences_get_int(SharedPreferences* thiz,const CHAR* key_path,INT32* ret);


/**
   Retrieve a double value from the preferences.
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @param [out] ret  save double value
   @return error code
 */
extern INT32 tg_shared_preferences_get_double(SharedPreferences* thiz,const CHAR* key_path,double* ret);

/**
   Retrieve a boolean value from the preferences.
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @param [out] ret  save bool value
   @return error code
 */
extern INT32 tg_shared_preferences_get_bool(SharedPreferences* thiz,const CHAR* key_path,BOOL* ret);

/**
    delete one item, if the item has sub item, the sub item also will be deleted
    @param [in] thiz SharedPreferences handle
    @param [in] key_path key path string
    @return error code
*/
extern INT32 tg_shared_preferences_delete_key(SharedPreferences* thiz,const CHAR* key_path);
 
/**
   put a serialization value into the preferences.
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @param [int] value serialization value
   @param [int] len serialization value length
   @return error code
 */
extern INT32 tg_shared_preferences_put_serialization(SharedPreferences* thiz,const CHAR* key_path,const void* value,INT32 len);


/**
   put a wstring value into the preferences.
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @param [int] value wstring value
   @return error code
 */
extern INT32 tg_shared_preferences_put_string_16(SharedPreferences* thiz,const CHAR* key_path,const WCHAR* value);


/**
   put a string value into the preferences.
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @param [int] value string value
   @return error code
 */
extern INT32 tg_shared_preferences_put_string(SharedPreferences* thiz,const CHAR* key_path,const CHAR* value);


/**
   put a int value into the preferences.
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @param [int] value int value
   @return error code
 */
extern INT32 tg_shared_preferences_put_int(SharedPreferences* thiz,const CHAR* key_path,INT32 value);

/**
   put a double value into the preferences.
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @param [int] value double value
   @return error code
 */
extern INT32 tg_shared_preferences_put_double(SharedPreferences* thiz,const CHAR* key_path,double value);

/**
   put a bool value into the preferences.
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @param [int] value bool value
   @return error code
 */
extern INT32 tg_shared_preferences_put_bool(SharedPreferences* thiz,const CHAR* key_path,BOOL value);

/**
   get child item count of one item
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @param [out] count save the sub item number
   @return error code
 */
extern INT32 tg_shared_preferences_get_child_count(SharedPreferences* thiz,const CHAR* key_path,INT32* count);

/**
   get child item full path
   @param [in] thiz SharedPreferences handle
   @param [in] key_path key path string
   @param [in] index index
   @param [out] ret save the sub item full path
   @return error code
 */
extern INT32 tg_shared_preferences_produce_child_path_by_idx(SharedPreferences* thiz,const CHAR* key_path,INT32 index,CHAR** ret);
#ifdef __cplusplus
}
#endif

#endif
