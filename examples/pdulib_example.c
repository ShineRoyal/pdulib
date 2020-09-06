#include <rtthread.h>
#include <pdulib.h>

#define DBG_TAG "pdu.eg"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

//char sms_data[256] = "0891683110804105F0240D91688103480636F80008028092719154231000610062006354C854C8003100320033";   //abc哈哈123
//char sms_data[256] = "0891683110804105F0240D91688103480636F80008028092313483230454C854C8";    //哈哈
char sms_data[256] = "0891683110804105F0240D91688103480636F80000028092512503231A61F1985C369FD169F59ADD76BFE171F99C5EB7DFF1793D"; //a-z

/**
 * 创建一个英文对象短信
 * @param argc
 * @param argv
 * @return
 */
int pdu_make_english_sms_example(int argc, char** argv)
{
    SMS_Submit_t *smsb_obj = RT_NULL;
    char *smsb_text = RT_NULL;

    smsb_obj = sms_submit_create_english_object("8618681688258", "Test");   //创建对象
    if (smsb_obj == RT_NULL)
    {
        LOG_E("sms_submit_create_object failed!");
        return;
    }

    smsb_text = sms_submit_create(*smsb_obj);                               //创建字符串
    LOG_HEX("test", 16, smsb_text, strlen(smsb_text));
    LOG_D("smsb_text=>%s", smsb_text);                                      //打印输出的PDU格式字符串

    return 0;
}

/**
 * 创建一个中文对象短信
 * @param argc
 * @param argv
 * @return
 */
int pdu_make_chinese_sms_example(int argc, char** argv)
{
    SMS_Submit_t *smsb_obj = RT_NULL;
    char *smsb_text = RT_NULL;

    //注意，这里的Test字串是用的中文格式存储，如果在这里直接输入中文，
    //可能会因为文本编辑器将中文转化为UTF8编码，并不是实际的USC2，从而接收机显示错误
    smsb_obj = sms_submit_create_chinese_object("8618681688258", "Test");   //创建对象
    if (smsb_obj == RT_NULL)
    {
        LOG_E("sms_submit_create_object failed!");
        return;
    }

    smsb_text = sms_submit_create(*smsb_obj);                               //创建字符串
    LOG_HEX("test", 16, smsb_text, strlen(smsb_text));
    LOG_D("smsb_text=>%s", smsb_text);                                      //打印输出的PDU格式字符串

    return 0;
}

/**
 * 解析一条带中文的短信，解析结果为英文
 * @param argc
 * @param argv
 * @return
 */
int pdu_parse_chinese_sms_example(int argc, char** argv)
{
    char sms_data[256] = "0891683110804105F0240D91688103480636F80008028092719154231000610062006354C854C8003100320033";   //abc哈哈123
    SMS_Deliver_t *sms = RT_NULL;
    sms = sms_deliver_parse(sms_data);
    LOG_D("sms data:%s\n", sms->user_data); //解析结果abc123
    sms_deliver_free(sms);
}

/**
 * 解析一条纯英文的短信
 * @param argc
 * @param argv
 * @return
 */
int pdu_parse_english_sms_example(int argc, char** argv)
{
    char sms_data[256] = "0891683110804105F0240D91688103480636F80000028092512503231A61F1985C369FD169F59ADD76BFE171F99C5EB7DFF1793D";  //a-z
    SMS_Deliver_t *sms = RT_NULL;
    sms = sms_deliver_parse(sms_data);
    LOG_D("sms data:%s\n", sms->user_data); //解析结果abcdefghijklmnopqrstuvwxyz
    sms_deliver_free(sms);
}

MSH_CMD_EXPORT(pdu_make_english_sms_example, pdu_make_english_sms_example);
MSH_CMD_EXPORT(pdu_make_chinese_sms_example, pdu_make_chinese_sms_example);
MSH_CMD_EXPORT(pdu_parse_chinese_sms_example, pdu_parse_chinese_sms_example);
MSH_CMD_EXPORT(pdu_parse_english_sms_example, pdu_parse_english_sms_example);

