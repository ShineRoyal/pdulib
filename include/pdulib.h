#ifndef __PDULIB_H_
#define __PDULIB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * 短信息中心
 */
typedef struct
{
    uint8_t len;                 //A：地址长度，2位十六进制数(1字节)。
    uint8_t type;                //B：号码类型，2位十六进制数。
    char* number;                //C：号码，B+C的长度将由A中的数据决定。
} SCA_t;

/**
 * 协议数据单元类型
 */
typedef union
{
    struct submit_t
    {
        uint8_t MTI :2;         //信息类型提示—TP-MTI（TP-Message-Type-Indicator）：00—读出（Deliver）;01—提交（Submit）
        uint8_t RD :1;          //拒绝复制—TP-RD（TP-Reject-Duplicates）：0—接受复制;1—拒绝复制
        uint8_t VPF :2;         //有效期格式—TP-VPF（TP-Validity-Period-Format）：00—不提供（Not present）;10—整型（标准）；01—预留;11—提供8位字节的一半（Semi-Octet Represented）
        uint8_t SRR :1;         //状态报告要求—TP-SPR（TP-Status-Report-Request）：0—需要报告;1—不需要报告
        uint8_t UDHI :1;        //用户数据头标识—TP-UDHL（TP-User-Data-Header-Indicator）：0—不含任何头信息;1—含头信息
        uint8_t RP :1;          //应答路径—TP-RP（TP-Reply-Path）：0—不设置;1—设置
    } submit;
    struct deliver_t
    {
        uint8_t MTI :2;         //信息类型提示—TP-MTI（TP-Message-Type-Indicator）：00—读出（Deliver）;01—提交（Submit）
        uint8_t MMS :1;         //有更多的信息需要发送（More Messages to Send）：0-有；1-没有，此值仅被短信息中心设置
        uint8_t Reserve :2;     //保留
        uint8_t SRI :1;         //状态报告要求—TP-SPR（TP-Status-Report-Request）：0—需要报告;1—不需要报告，一般为0
        uint8_t UDHI :1;        //用户数据头标识—TP-UDHL（TP-User-Data-Header-Indicator）：0—不含任何头信息;1—含头信息
        uint8_t RP :1;          //应答路径—TP-RP（TP-Reply-Path）：0—不设置;1—设置。一般为0
    } deliver;
    uint8_t byte;               //byte形式访问的数据
} PDUType_t;

/**
 * 消息参考
 */
typedef union
{
    uint8_t byte;               //byte形式访问的数据
} MR_t;

/**
 * 发送方地址
 */
typedef struct
{
    uint8_t len;                //F：被叫号码长度，2位十六进制数。
    uint8_t type;               //G：被叫号码类型，2位十六进制数，取值同B。
    char* number;               //H：被叫号码，长度由F中的数据决定。
} OA_DA_t;

/**
 * PID协议标识
 */
typedef union
{
    uint8_t byte;
} PID_t;

/**
 * 数据编码解决方案
 */
typedef union
{
    struct
    {
        uint8_t Class :2;       //短信类型：00-直接显示在屏幕上；01-ME特定信息；10-SIM卡特定信息；11-TE特定信息
        uint8_t Encode :2;      //编码方案：00-默认字母表7bit；01-8bit；10-USC2双字节字符集；11-保留
        uint8_t ClassEnable :1; //0-Class保留，不含信息类型；1-Class含有信息类型。
        uint8_t Compress :1;    //0-文本未压缩；1-文本用GSM标准压缩算法压缩。
        uint8_t Reserve :1;     //一般设置为00
    } dcs_t;
    uint8_t byte;
} DCS_t;

/**
 * 信息有效期
 */
typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t timezone;
} SCTS_VP_t;

/**
 * 发送方PDU格式
 */
typedef struct
{
    SCA_t sca;
    PDUType_t pdu;
    MR_t mr;
    OA_DA_t da;
    PID_t pid;
    DCS_t dcs;
    union
    {
        SCTS_VP_t abs;
        uint32_t rel;
    } vp;
    uint8_t user_data_length;
    char *user_data;
} SMS_Submit_t;

/**
 * 接收方PDU格式
 */
typedef struct
{
    SCA_t sca;
    PDUType_t pdu;
    OA_DA_t oa;
    PID_t pid;
    DCS_t dcs;
    SCTS_VP_t scts;
    uint8_t user_data_length;
    char *user_data;
} SMS_Deliver_t;

//快速创建英文短信的SMS对象
SMS_Submit_t* sms_submit_create_english_object(const char *da_number, const char *user_data);
//快速创建中文短信的SMS对象
SMS_Submit_t* sms_submit_create_chinese_object(const char *da_number, const char *user_data);
//将SMS对象转成PDU格式字符串
char *sms_submit_create(SMS_Submit_t sms);
//解析收到的PDU格式短信字符串为SMS对象
SMS_Deliver_t* sms_deliver_parse(const char *data);
//释放对象
void sms_deliver_free(SMS_Deliver_t *sms);
//释放对象
void sms_submit_free(SMS_Submit_t *sms);
#endif
