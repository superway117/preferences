
#include "CUnit.h"
#include "Automated.h"

#include "../shared_preferences.h"


static int sharedpreferences_test_suit_init(void)
{
    tg_shared_preferences_manager_init();
    tg_shared_preferences_init_from_string("/nvram/sp_init_from_file_default.conf","{ \"abc\": 12, \
\"pai\": 3.14, \"uni_str\":\"f182de98cc51\",\"uni_str1\":\"32003300\",\"bool0\": false, \"bool1\": true, \"arrs\": {\"align\":\"center\", \"height\":240},\"table\": {\"align\":\"center\",\"size\":{\"height\":112,\"width\":240}} }");
    return 0;
}
static int sharedpreferences_test_suit_clean(void)
{
    tg_heap_traverse(FILE_PRINT);
    return 0;
}


static void sharedpreferences_init_from_string_test()
{
    SharedPreferences* shared=NULL;

    INT32 ret = 0;
    INT32 int_value;
    double double_value;
    BOOL bool_value;
    WCHAR* uni_str = NULL;
    WCHAR* uni_str1 = NULL;
    CHAR* str=NULL;
    CU_ASSERT_EQUAL(tg_shared_preferences_init_from_string("/nvram/sp_init_from_string.conf","{ \"abc\": 12, \
 \"pai\": 3.14, \"uni_str\":\"f182de98cc51\",\"uni_str1\":\"32003300\",\"bool0\": false, \"bool1\": true, \"arrs\": {\"align\":\"center\", \"height\":240},\"table\": {\"align\":\"center\",\"size\":{\"height\":112,\"width\":240}} }"),0);
    shared=tg_shared_preferences_open("/nvram/sp_init_from_string.conf",SharedPreferences_Read_Mode);
    if (shared==NULL)
        return;
    ret = tg_shared_preferences_get_bool(shared,"bool0",&bool_value);
    CU_ASSERT_EQUAL(bool_value,FALSE);



    ret = tg_shared_preferences_get_bool(shared,"/bool1",&bool_value);
    CU_ASSERT_EQUAL(bool_value,TRUE);

    ret = tg_shared_preferences_get_double(shared,"pai",&double_value);
    CU_ASSERT_EQUAL(double_value,3.14);

    ret= tg_shared_preferences_get_string_16(shared,"uni_str",&uni_str);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    if (ret==SharedPreferences_SUCC)
    {
        CU_ASSERT_EQUAL((wstrcmp(uni_str, L"Ó¢·ÉÁè")),0);
        TG_FREE(uni_str);
    }

    ret= tg_shared_preferences_get_string_16(shared,"uni_str1",&uni_str1);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    if (ret==SharedPreferences_SUCC)
    {
        CU_ASSERT_EQUAL((wstrcmp(uni_str1,L"23")),0);
        TG_FREE(uni_str1);
    }
    ret = tg_shared_preferences_get_string(shared,"/arrs/align",&str);

    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    if (ret==SharedPreferences_SUCC)
    {
        CU_ASSERT_EQUAL((strcmp(str,"center")),0);

        TG_FREE(str);
    }

    ret = tg_shared_preferences_get_int(shared,"arrs/height",&int_value);
    CU_ASSERT_EQUAL(int_value,240);
    CU_ASSERT_EQUAL(tg_shared_preferences_close(shared),SharedPreferences_SUCC);

}


static void sharedpreferences_init_from_file_test()
{

    SharedPreferences *shared=NULL;

    INT32 ret = 0;
    INT32 int_value;
    double double_value;
    BOOL bool_value;
    WCHAR* uni_str = NULL;
    WCHAR* uni_str1 = NULL;
    CHAR* str=NULL;
    void* serial_obj = NULL;

    if (tg_shared_preferences_init_from_file("/nvram/sp_init_from_file.conf","/nvram/sp_init_from_file_default.conf") != 0)
        return;
    shared=tg_shared_preferences_open("/nvram/sp_init_from_file.conf",SharedPreferences_Read_Mode);
    if (shared==NULL)
        return;
    ret = tg_shared_preferences_get_int(shared,"abc",&int_value);
    CU_ASSERT_EQUAL(int_value,12);



    ret = tg_shared_preferences_get_bool(shared,"bool0",&bool_value);
    CU_ASSERT_EQUAL(bool_value,FALSE);

    ret = tg_shared_preferences_get_bool(shared,"/bool1",&bool_value);
    CU_ASSERT_EQUAL(bool_value,TRUE);

    ret = tg_shared_preferences_get_double(shared,"pai",&double_value);
    CU_ASSERT_EQUAL(double_value,3.14);

    ret= tg_shared_preferences_get_string_16(shared,"uni_str",&uni_str);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    if (ret==SharedPreferences_SUCC)
    {
        //CU_ASSERT_EQUAL((memcmp(uni_str,"\u82f1\u98de\u51cc",wstrlen(uni_str)*2)),0);
        CU_ASSERT_EQUAL((wstrcmp(uni_str, L"Ó¢·ÉÁè")),0);
        TG_FREE(uni_str);
    }

    ret= tg_shared_preferences_get_string_16(shared,"uni_str1",&uni_str1);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    if (ret==SharedPreferences_SUCC)
    {
        CU_ASSERT_EQUAL((wstrcmp(uni_str1,L"23")),0);
        TG_FREE(uni_str1);
    }

    ret = tg_shared_preferences_get_bool(shared,"/wrongkey1/wrongkey2",&bool_value);
    CU_ASSERT_EQUAL(ret,SharedPreferences_KEY_NOT_EXIST);

    ret = tg_shared_preferences_get_bool(shared,"/wrongkey1",&bool_value);
    CU_ASSERT_EQUAL(ret,SharedPreferences_KEY_NOT_EXIST);


    ret = tg_shared_preferences_get_string(shared,"/arrs/align",&str);

    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    if (ret==SharedPreferences_SUCC)
    {
        CU_ASSERT_EQUAL((strcmp(str,"center")),0);

        TG_FREE(str);
    }
    str = (CHAR*)tg_shared_preferences_get_string_v2(shared,"/arrs/align");
    if (str)
    {
        CU_ASSERT_EQUAL((strcmp(str,"center")),0);
    }

    ret = tg_shared_preferences_get_int(shared,"arrs/height",&int_value);
    CU_ASSERT_EQUAL(int_value,240);
    ret = tg_shared_preferences_get_serialization(shared,"serial",&serial_obj);
    if (ret==SharedPreferences_SUCC)
    {
        UINT32* addr=(UINT32*)serial_obj;
        CU_ASSERT_EQUAL((*addr),0x412800af);
        TG_FREE(serial_obj);
    }

    CU_ASSERT_EQUAL(tg_shared_preferences_close(shared),SharedPreferences_SUCC);


}

static void sharedpreferences_delete_test()
{

    SharedPreferences *shared=NULL;

    INT32 ret = 0;
    INT32 int_value;

    BOOL bool_value;
    if (tg_shared_preferences_init_from_file("/nvram/sp_init_from_file.conf","/nvram/sp_init_from_file_default.conf") != 0)
        return;
    shared=tg_shared_preferences_open("/nvram/sp_init_from_file.conf",SharedPreferences_ReadWrite_Mode);
    if (shared==NULL)
        return;
    ret = tg_shared_preferences_delete_key(shared,"/abc");
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);

    ret = tg_shared_preferences_get_int(shared,"abc",&int_value);
    CU_ASSERT_EQUAL(ret,SharedPreferences_KEY_NOT_EXIST);

    ret = tg_shared_preferences_delete_key(shared,"/arrs/height");
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);

    ret = tg_shared_preferences_get_int(shared,"/arrs/height",&int_value);
    CU_ASSERT_EQUAL(ret,SharedPreferences_KEY_NOT_EXIST);


    ret = tg_shared_preferences_get_int(shared,"table/size/height/",&int_value);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    CU_ASSERT_EQUAL(int_value,112);

    ret = tg_shared_preferences_delete_key(shared,"table/size/height");
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);

    ret = tg_shared_preferences_get_int(shared,"table/size/height/",&int_value);
    CU_ASSERT_EQUAL(ret,SharedPreferences_KEY_NOT_EXIST);


    tg_shared_preferences_commit(shared);
    CU_ASSERT_EQUAL(tg_shared_preferences_close(shared),0);
    shared=tg_shared_preferences_open("/nvram/sp_init_from_file.conf",SharedPreferences_ReadWrite_Mode);
    if (shared==NULL)
        return;

    ret = tg_shared_preferences_get_int(shared,"abc",&int_value);
    CU_ASSERT_EQUAL(ret,SharedPreferences_KEY_NOT_EXIST);



    ret = tg_shared_preferences_get_bool(shared,"bool0",&bool_value);
    CU_ASSERT_EQUAL(bool_value,FALSE);

    ret = tg_shared_preferences_get_bool(shared,"/bool1",&bool_value);
    CU_ASSERT_EQUAL(bool_value,TRUE);

    ret = tg_shared_preferences_get_bool(shared,"/wrongkey1/wrongkey2",&bool_value);
    CU_ASSERT_EQUAL(ret,SharedPreferences_KEY_NOT_EXIST);

    ret = tg_shared_preferences_get_bool(shared,"/wrongkey1",&bool_value);
    CU_ASSERT_EQUAL(ret,SharedPreferences_KEY_NOT_EXIST);

    CU_ASSERT_EQUAL(strcmp(tg_shared_preferences_get_string_v2(shared,"/arrs/align"),"center"),0);

    ret = tg_shared_preferences_get_int(shared,"arrs/height",&int_value);
    CU_ASSERT_EQUAL(ret,SharedPreferences_KEY_NOT_EXIST);

    ret = tg_shared_preferences_get_int(shared,"table/size/height/",&int_value);
    CU_ASSERT_EQUAL(ret,SharedPreferences_KEY_NOT_EXIST);

    CU_ASSERT_EQUAL(tg_shared_preferences_close(shared),SharedPreferences_SUCC);


}

static void sharedpreferences_open_new_file_test()
{

    SharedPreferences *shared=NULL;

    INT32 ret = 0;
    INT32 int_value;
    BOOL bool_value;

    shared=tg_shared_preferences_open("/nvram/sp_no_init_file.conf",SharedPreferences_ReadWrite_Mode);
    if (shared==NULL)
        return;
    ret = tg_shared_preferences_get_bool(shared,"bool0",&bool_value);
    CU_ASSERT_EQUAL(ret,SharedPreferences_KEY_NOT_EXIST);

    CU_ASSERT_EQUAL((tg_shared_preferences_put_string(shared,"/arrs","left")),SharedPreferences_SUCC);
    CU_ASSERT_EQUAL(strcmp(tg_shared_preferences_get_string_v2(shared,"/arrs"),"left"),SharedPreferences_SUCC);

    CU_ASSERT_EQUAL(tg_shared_preferences_put_int(shared,"table/size/height/",120),SharedPreferences_SUCC)
    ret = tg_shared_preferences_get_int(shared,"table/size/height/",&int_value);

    ret = tg_shared_preferences_get_int(shared,"arrs/height",&int_value);
    CU_ASSERT_EQUAL(ret,SharedPreferences_KEY_NOT_EXIST);

    CU_ASSERT_EQUAL((tg_shared_preferences_put_string(shared,"/table/name","mytable")),SharedPreferences_SUCC);
    CU_ASSERT_EQUAL(strcmp(tg_shared_preferences_get_string_v2(shared,"/table/name"),"mytable"),SharedPreferences_SUCC);

    tg_shared_preferences_commit(shared);
    CU_ASSERT_EQUAL(tg_shared_preferences_close(shared),SharedPreferences_SUCC);

}

static void sharedpreferences_put_test()
{

    SharedPreferences *shared=NULL;
    UINT8 blob[10];
    INT32 ret = 0;
    INT32 int_value;
    double double_value;
    BOOL bool_value;
    WCHAR* uni_str = NULL;
    void* serial_obj = NULL;
    if (tg_shared_preferences_init_from_file("/nvram/sp_init_from_file.conf","/nvram/sp_init_from_file_default.conf") != 0)
        return;
    shared=tg_shared_preferences_open("/nvram/sp_init_from_file.conf",SharedPreferences_ReadWrite_Mode);
    if (shared==NULL)
        return;



    ret = tg_shared_preferences_get_bool(shared,"bool0",&bool_value);
    CU_ASSERT_EQUAL(bool_value,FALSE);

    ret = tg_shared_preferences_get_bool(shared,"/bool1",&bool_value);
    CU_ASSERT_EQUAL(bool_value,TRUE);

    CU_ASSERT_EQUAL(strcmp(tg_shared_preferences_get_string_v2(shared,"/arrs/align"),"center"),SharedPreferences_SUCC);
    CU_ASSERT_EQUAL((tg_shared_preferences_put_string(shared,"/arrs/align","left")),SharedPreferences_SUCC);
    CU_ASSERT_EQUAL(strcmp(tg_shared_preferences_get_string_v2(shared,"/arrs/align"),"left"),SharedPreferences_SUCC);

    ret = tg_shared_preferences_get_int(shared,"arrs/height",&int_value);
    CU_ASSERT_EQUAL(int_value,240);
    CU_ASSERT_EQUAL(tg_shared_preferences_put_int(shared,"arrs/height",480),SharedPreferences_SUCC);

    ret = tg_shared_preferences_get_int(shared,"arrs/height",&int_value);
    CU_ASSERT_EQUAL(int_value,480);

    ret = tg_shared_preferences_get_int(shared,"/table/size/height/",&int_value);
    CU_ASSERT_EQUAL(int_value,112);
    CU_ASSERT_EQUAL(tg_shared_preferences_put_int(shared,"table/size/height/",120),SharedPreferences_SUCC);

    CU_ASSERT_EQUAL(tg_shared_preferences_put_string_16(shared,"uni_str",L"Ó¢ÌØ¶û"),SharedPreferences_SUCC);
    ret= tg_shared_preferences_get_string_16(shared,"uni_str",&uni_str);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    if (ret==SharedPreferences_SUCC)
    {
        //CU_ASSERT_EQUAL((memcmp(uni_str,"\u82f1\u98de\u51cc",wstrlen(uni_str)*2)),0);
        CU_ASSERT_EQUAL((wstrcmp(uni_str, L"Ó¢ÌØ¶û")),0);
        TG_FREE(uni_str);
    }
    CU_ASSERT_EQUAL(tg_shared_preferences_put_string_16(shared,"new_str",L"Ó¢ÌØ¶û"),SharedPreferences_SUCC);
    ret= tg_shared_preferences_get_string_16(shared,"new_str",&uni_str);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    if (ret==SharedPreferences_SUCC)
    {
        //CU_ASSERT_EQUAL((memcmp(uni_str,"\u82f1\u98de\u51cc",wstrlen(uni_str)*2)),0);
        CU_ASSERT_EQUAL((wstrcmp(uni_str, L"Ó¢ÌØ¶û")),0);
        TG_FREE(uni_str);
    }
    int_value = (INT32)sharedpreferences_put_test;
    ret = tg_shared_preferences_put_serialization(shared,"serial",&int_value,4);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);

    for (int_value=0;int_value<10;int_value++)
	 blob[int_value]=(UINT8)int_value;
    ret = tg_shared_preferences_put_serialization(shared,"serial1",blob,10);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);


    tg_shared_preferences_commit(shared);
    CU_ASSERT_EQUAL(tg_shared_preferences_close(shared),SharedPreferences_SUCC);


//reopen,get again
    shared=tg_shared_preferences_open("/nvram/sp_init_from_file.conf",SharedPreferences_Read_Mode);
    if (shared==NULL)
        return;

    ret = tg_shared_preferences_get_double(shared,"pai",&double_value);
    CU_ASSERT_EQUAL(double_value,3.14);

    ret= tg_shared_preferences_get_string_16(shared,"uni_str",&uni_str);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    if (ret==SharedPreferences_SUCC)
    {
        //CU_ASSERT_EQUAL((memcmp(uni_str,"\u82f1\u98de\u51cc",wstrlen(uni_str)*2)),0);
        CU_ASSERT_EQUAL((wstrcmp(uni_str, L"Ó¢ÌØ¶û")),0);
        TG_FREE(uni_str);
    }

    ret= tg_shared_preferences_get_string_16(shared,"uni_str1",&uni_str);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    if (ret==SharedPreferences_SUCC)
    {
        CU_ASSERT_EQUAL((wstrcmp(uni_str,L"23")),0);
        TG_FREE(uni_str);
    }

    ret= tg_shared_preferences_get_string_16(shared,"new_str",&uni_str);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    if (ret==SharedPreferences_SUCC)
    {
        CU_ASSERT_EQUAL((wstrcmp(uni_str,L"Ó¢ÌØ¶û")),0);
        TG_FREE(uni_str);
    }

    int_value = (INT32)sharedpreferences_put_test;
    ret = tg_shared_preferences_get_serialization(shared,"serial",&serial_obj);
    if (ret==SharedPreferences_SUCC)
    {
        UINT32* addr=(UINT32*)serial_obj;
        CU_ASSERT_EQUAL((*addr),int_value);
        TG_FREE(serial_obj);
    }


    ret = tg_shared_preferences_get_serialization(shared,"serial1",&serial_obj);
    if (ret==SharedPreferences_SUCC)
    {
        UINT8* addr=(UINT8*)serial_obj;
        for (int_value=0;int_value<10;int_value++)
			CU_ASSERT_EQUAL(blob[int_value],int_value);
        TG_FREE(serial_obj);
    }

    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);

    CU_ASSERT_EQUAL(tg_shared_preferences_close(shared),SharedPreferences_SUCC);
}

static void sharedpreferences_child_test()
{

    SharedPreferences* shared=NULL;
    INT32 ret = 0;
    INT32 count;
    CHAR* child_path = NULL;
    if (tg_shared_preferences_init_from_file("/nvram/sp_init_from_file.conf","/nvram/sp_init_from_file_default.conf") != 0)
        return;
    shared=tg_shared_preferences_open("/nvram/sp_init_from_file.conf",SharedPreferences_ReadWrite_Mode);
    if (shared==NULL)
        return;
    ret= tg_shared_preferences_get_child_count(shared,"/",&count);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    CU_ASSERT_EQUAL(count,8);

    ret= tg_shared_preferences_get_child_count(shared,"/abc",&count);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    CU_ASSERT_EQUAL(count,0);

    ret= tg_shared_preferences_get_child_count(shared,"/table/size",&count);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    CU_ASSERT_EQUAL(count,2);

    ret =  tg_shared_preferences_produce_child_path_by_idx(shared,"/table/size",0,&child_path);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    if (child_path)
    {
        CU_ASSERT_EQUAL((strcmp(child_path, "/table/size/height")),0);
        TG_FREE(child_path);
    }

    ret =  tg_shared_preferences_produce_child_path_by_idx(shared,"/",0,&child_path);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    if (child_path)
    {
        CU_ASSERT_EQUAL((strcmp(child_path, "/abc")),0);
        TG_FREE(child_path);
    }

    CU_ASSERT_EQUAL(tg_shared_preferences_close(shared),SharedPreferences_SUCC);
}

static CU_TestInfo sharedpreferences_init_from_str_testcases[] =
{
    //  {"sharedpreferences_init_from_null_string_test",sharedpreferences_init_from_null_string_test},
    {"sharedpreferences_init_from_str_testcases:", sharedpreferences_init_from_string_test},

    CU_TEST_INFO_NULL
};
static CU_TestInfo sharedpreferences_init_from_file_testcases[] =
{

    {"sharedpreferences_init_from_file_testcases:", sharedpreferences_init_from_file_test},
    CU_TEST_INFO_NULL
};
static CU_TestInfo sharedpreferences_delete_testcases[] =
{
    {"sharedpreferences_delete_testcases:", sharedpreferences_delete_test},
    CU_TEST_INFO_NULL
};

static CU_TestInfo sharedpreferences_put_testcases[] =
{
    {"sharedpreferences_put_testcases:", sharedpreferences_put_test},
    {"sharedpreferences_open_new_file_test",sharedpreferences_open_new_file_test},
    CU_TEST_INFO_NULL
};


static CU_TestInfo sharedpreferences_child_testcases[] =
{
    {"sharedpreferences_child_testcases:", sharedpreferences_child_test},
    CU_TEST_INFO_NULL
};


static void sharedpreferences_lock_test()
{
    SharedPreferences* shared_read;
    SharedPreferences* shared_write;
    SharedPreferences* shared_write1;
    INT32 error_read = 0;
    if (tg_shared_preferences_init_from_file("/nvram/sp_init_from_file.conf","/nvram/sp_init_from_file_default.conf") != 0)
        return;
    shared_read=tg_shared_preferences_open("/nvram/sp_init_from_file.conf",SharedPreferences_Read_Mode);
    if (shared_read==NULL)
        return;
    shared_write = tg_shared_preferences_open("/nvram/sp_init_from_string.conf",SharedPreferences_ReadWrite_Mode);
    shared_write1 = tg_shared_preferences_open("/nvram/sp_init_from_string.conf",SharedPreferences_ReadWrite_Mode);

    CU_ASSERT_EQUAL( shared_write1,NULL);

    CU_ASSERT_EQUAL(tg_shared_preferences_close(shared_read),0);
    if (shared_write!=0)
        CU_ASSERT_EQUAL(tg_shared_preferences_close(shared_write),0);
    if (shared_write1!=0)
        CU_ASSERT_EQUAL(tg_shared_preferences_close(shared_write1),0);

}


static CU_TestInfo sharedpreferences_lock_testcases[] =
{
    {"sharedpreferences_lock_test:", sharedpreferences_lock_test},
    CU_TEST_INFO_NULL
};



INT32 sp_notify_cb1(  )
{
    return 0;
}
INT32 sp_notify_cb2(  )
{
    return 0;
}

INT32 sp_notify_cb3(  )
{
    return 0;
}

INT32 sp_notify_cb4(  )
{
    return 0;
}
static void sharedpreferences_register_test()
{
    INT32 ret = 0;
    SharedPreferences* shared=NULL;
    if (tg_shared_preferences_init_from_file("/nvram/sp_init_from_file.conf","/nvram/sp_init_from_file_default.conf") != 0)
        return;
    shared=tg_shared_preferences_open("/nvram/sp_init_from_file.conf",SharedPreferences_ReadWrite_Mode);
    if (shared==NULL)
        return;
    CU_ASSERT_EQUAL( tg_shared_preferences_register("/nvram/sp_init_from_file.conf","table/size/height/",sp_notify_cb1,SharedPreferences_Modify_Mode),0);
    CU_ASSERT_EQUAL( tg_shared_preferences_register("/nvram/sp_init_from_file.conf","table/size/height/",sp_notify_cb2,SharedPreferences_Add_Mode),0);
    CU_ASSERT_EQUAL( tg_shared_preferences_register("/nvram/sp_init_from_file.conf","table/size/height/",sp_notify_cb3,SharedPreferences_Delete_Mode),0);
    CU_ASSERT_EQUAL( tg_shared_preferences_register("/nvram/sp_init_from_file.conf","table/size/",sp_notify_cb4,SharedPreferences_Delete_Mode),0);

    ret = tg_shared_preferences_put_int(shared,"table/size/height/",120);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    ret = tg_shared_preferences_delete_key(shared,"table/size/height/");
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);
    ret = tg_shared_preferences_put_int(shared,"table/size/height/",140);
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);

    ret = tg_shared_preferences_delete_key(shared,"table/size/");
    CU_ASSERT_EQUAL(ret,SharedPreferences_SUCC);

    CU_ASSERT_EQUAL(tg_shared_preferences_close(shared),0);
    tg_shared_preferences_unregister_all();
}
static CU_TestInfo sharedpreferences_register_testcases[] =
{
    {"sharedpreferences_register_test:", sharedpreferences_register_test},
    CU_TEST_INFO_NULL
};

static CU_SuiteInfo sharedpreferences_suites[] = {
    {"Testing sharedpreferences_init_from_str_testcases:", sharedpreferences_test_suit_init, NULL, sharedpreferences_init_from_str_testcases},
    {"Testing sharedpreferences_init_from_file_testcases:", NULL, NULL, sharedpreferences_init_from_file_testcases},
    {"Testing sharedpreferences_delete_testcases:", NULL, NULL, sharedpreferences_delete_testcases},
    {"Testing sharedpreferences_put_testcases:", NULL, NULL, sharedpreferences_put_testcases},

    {"Testing sharedpreferences_register_testcases:", NULL, NULL, sharedpreferences_register_testcases},
    {"Testing sharedpreferences_child_testcases:", NULL, NULL, sharedpreferences_child_testcases},
    {"Testing sharedpreferences_lock_testcases:", NULL, sharedpreferences_test_suit_clean, sharedpreferences_lock_testcases},
    CU_SUITE_INFO_NULL
};

static void sp_add_tests(void)
{
    CU_get_registry();
    if (CU_is_test_running())
        return;


    if (CUE_SUCCESS != CU_register_suites(sharedpreferences_suites))
    {
        return;
    }
}

void sp_test_run()
{
    if (CU_initialize_registry())
    {
        return;
    }
    else
    {
        sp_add_tests();
        CU_set_output_filename("sharedpreferences_test_output");
        CU_list_tests_to_file();
        CU_automated_run_tests();
        CU_cleanup_registry();
    }
}
