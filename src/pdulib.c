#include <rtthread.h>
#include <pdulib.h>

#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#define DBG_TAG "pdulib"
#ifdef PDULIB_DEBUG
#define DBG_LVL DBG_LOG
#else
#define DBG_LVL DBG_INFO
#endif

#include <rtdbg.h>

/**
 * hex字符串转uint8_t类型数据
 * @param addr      字符串首地址
 * @param value     转换结束的值
 * @return  成功返回RT_EOK
 */
static int hexstr2uint8_t(const char *addr, uint8_t *value)
{
    uint8_t ret = RT_EOK;
    char *temp = RT_NULL;

    temp = rt_calloc(1, 3);
    if (temp == RT_NULL)
    {
        LOG_E("no memery for hexstring to uint8_t");
        ret = -RT_ENOMEM;
        goto __exit;
    }
    strncpy(temp, addr, 2);
    sscanf(temp, "%X", value);
    ret = RT_EOK;

    __exit:
    //
    rt_free(temp);
    return ret;
}

/**
 * dec字符串转uint8_t类型数据,仅用于处理SCTS时钟数据
 * @param addr      字符串首地址
 * @param value     转换结束的值
 * @return  成功返回RT_EOK
 */
static int decstr2uint8_t(const char *addr, uint8_t *value)
{
    uint8_t ret = RT_EOK;
    *value = (addr[0] - '0') + (addr[1] - '0') * 10;
    return ret;
}

/**
 * hex字符串转号码字符串
 * @param addr      字符串首地址
 * @param length    字符串长度
 * @return  转换后的号码字符串指针
 */
static char* hexstr2number(const char *addr, uint8_t length)
{
    uint8_t i = 0;
    char *str = RT_NULL;
    str = rt_calloc(1, length + 1);
    for (i = 0; i < length; i = i + 2)
    {
        str[i + 1] = addr[i];
        str[i] = addr[i + 1];
    }
    if (str[i - 1] == 'F')
        str[i - 1] = 0;
    if (str[i - 2] == 'F')
        str[i - 2] = 0;

    return str;

}

/**
 * bit7压缩格式字符串还原成ascii字串
 * @param bit7str       待解码的压缩字符串
 * @param uint8_tstr    解码后的字符串
 * @param length        待解码的压缩字符串长度
 * @return  返回转换完成的uint8_tstr字串长度
 */
static size_t bit7str2uint8_tstr(const char *bit7str, char *uint8_tstr, int length)
{
    uint8_t bit_pos = 0;

    uint8_t s7_pos = 0; //bit7str字串索引
    uint8_t s8_pos = 0; //uint8_tstr字串索引

    uint8_tstr[s8_pos] = bit7str[s7_pos] & 0x7F;

    for (bit_pos = 1, s8_pos = 1;; s8_pos++, bit_pos++)
    {
        if (s7_pos >= length)
            break;

        if (bit_pos % 8 == 0)   //第8*n次不需要移位
            bit_pos = 0;
        else
            s7_pos++;           //其他时候都是跨byte的

        uint8_t big = (bit7str[s7_pos] << bit_pos);
        uint8_t small = (bit7str[s7_pos - 1] >> (8 - bit_pos));
        uint8_tstr[s8_pos] = (big | small) & 0x7F;
        //当数据量较多时，此处使用logd输出调试信息会卡死
        //rt_kprintf("s7_pos=>%d,bit_pos=>%d,big=>%x,small=>%x,uint8_tstr[%d]=>%x\n", s7_pos, bit_pos, big, small, s8_pos, uint8_tstr[s8_pos]);
    }
    return s8_pos;
}

/**
 * usc2字符串过滤中文，还原成ascii字串
 * @param usc2str       待解码的字符串
 * @param uint8_tstr    解析完的字符串
 * @param length        待解码字符串长度
 * @return  返回转换完成的uint8_tstr字串长度
 */
static size_t usc2str2uint8_tstr(const char *usc2str, char *uint8_tstr, int length)
{
    uint8_t s2_pos = 0; //usc2str字串索引
    uint8_t s8_pos = 0; //uint8_tstr字串索引
    for (; s2_pos < length; s2_pos = s2_pos + 2)
    {
        if (usc2str[s2_pos] != 0)  //高位不是0，中文
            continue;
        uint8_tstr[s8_pos] = usc2str[s2_pos + 1];
        s8_pos++;
    }
    return s2_pos;
}

/**
 * ascii字串压缩成bit7格式字符串
 * @param uint8_tstr    待压缩的字符串首地址
 * @param bit7str       压缩后的字符串首地址
 * @param length        待压缩的字符串长度
 * @return  压缩后的长度
 */
static size_t uint8_tstr2bit7str(const char *uint8_tstr, char *bit7str, int length)
{
    uint8_t bit_pos = 0;

    uint8_t s7_pos = 0; //bit7str字串索引
    uint8_t s8_pos = 0; //uint8_tstr字串索引
    LOG_D("using bit7 encoding...");
    for (;; s8_pos++, bit_pos++)
    {
        if (s8_pos >= length)
            break;

        if (bit_pos % 7 == 0)   //第7*n次不需要移位
            bit_pos = 0;
        else
            s7_pos++;           //其他时候都是跨byte的

        uint8_t small = uint8_tstr[s8_pos] >> bit_pos;
        uint8_t big = uint8_tstr[s8_pos + 1] << (7 - bit_pos);
        bit7str[s7_pos] = small | big;

        //当数据量较多时，此处使用logd输出调试信息会卡死
        //rt_kprintf("s7_pos=>%d,bit_pos=>%d,big=>%x,small=>%x,uint8_tstr[%d]=>%x\n", s7_pos, bit_pos, big, small, s8_pos, uint8_tstr[s8_pos]);
    }
    return s7_pos + 1;
}

/**
 * ascii字串转usc2
 * @param uint8_tstr    待编码的字符串首地址
 * @param usc2str       编码后的字符串首地址
 * @param length        待编码的字符串长度
 * @return  编码后的字符串长度
 */
static size_t uint8_tstr2usc2str(const char *uint8_tstr, char *usc2str, int length)
{
    uint8_t s2_pos = 0; //usc2str字串索引
    uint8_t s8_pos = 0; //uint8_tstr字串索引
    LOG_D("using usc2 encoding...");
    for (; s8_pos < length; s8_pos++, s2_pos += 2)
    {
        usc2str[s2_pos] = 0;
        usc2str[s2_pos + 1] = uint8_tstr[s8_pos];
    }
    return s2_pos + 1;
}

/**
 * 解析SCA短信息中心地址信息
 * @param data  待解析数据首地址
 * @param sca   解析结果
 * @return 解析成功返回RT_EOK
 */
static rt_err_t get_SCA_info(const char *data, SCA_t *sca)
{
    rt_err_t ret = RT_EOK;
    uint8_t number_len = 0;
    char *ptr = data;

    hexstr2uint8_t(ptr, &sca->len);
    if (sca->len != 8)                //通常情况下长度为8，其他情况先不考虑
    {
        ret = RT_EINVAL;
        goto __exit;
    }
    LOG_D("sca->len=>%d", sca->len);

    ptr += 2;
    hexstr2uint8_t(ptr, &sca->type);
    if (sca->type & 0x80 == 0)    //最高位应该为1
    {
        //bit6-bit4:数值类型（Type of Number）：000—未知，001—国际，010—国内,111—留作扩展；
        //bit3-bit0:号码鉴别（Numbering plan identification）:0000—未知，0001—ISDN/电话号码(E.164/E.163)，1111—留作扩展；
        ret = RT_EINVAL;
        goto __exit;
    }
    LOG_D("sca->type=>0x%x", sca->type);

    ptr += 2;
    number_len = sca->len - 1;                              //短信息中心号码=长度-类型长度(1字节)
    sca->number = hexstr2number(ptr, number_len * 2);       //1字节=2字符
    if (sca->number == RT_NULL)
    {
        ret = RT_EINVAL;
        goto __exit;
    }
    LOG_D("sca->number=>%s", sca->number);

    __exit:
//错误处理
    return ret;

}

/**
 * 解析协议数据单元类型
 * @param data  待解析数据首地址
 * @param pdu   解析结果
 * @return 解析成功返回RT_EOK
 */
static rt_err_t get_PDUType_info(const char *data, PDUType_t *pdu)
{
    rt_err_t ret = RT_EOK;
    char *ptr = data;
    hexstr2uint8_t(ptr, &pdu->byte);
    LOG_D("pdu->byte=>0x%x", pdu->byte);
    return ret;
}

/**
 * 解析OA/DA发送方地址/接收方地址信息
 * @param data  待解析数据首地址
 * @param oada  解析结果
 * @return 解析成功返回RT_EOK
 */
static rt_err_t get_OADA_info(const char *data, OA_DA_t *oada)
{
    rt_err_t ret = RT_EOK;
    uint8_t number_len = 0;
    char *ptr = data;

    hexstr2uint8_t(ptr, &oada->len);
    LOG_D("oada->len=>%d", oada->len);

    ptr += 2;
    hexstr2uint8_t(ptr, &oada->type);
    if (oada->type & 0x80 == 0)    //最高位应该为1
    {
        //bit6-bit4:数值类型（Type of Number）：000—未知，001—国际，010—国内,111—留作扩展；
        //bit3-bit0:号码鉴别（Numbering plan identification）:0000—未知，0001—ISDN/电话号码(E.164/E.163)，1111—留作扩展；
        ret = RT_EINVAL;
        goto __exit;
    }
    LOG_D("oada->type=>0x%x", oada->type);

    ptr += 2;
    number_len = oada->len;                                 //短信息中心号码=长度-类型长度(1字节)
    if (number_len % 2)
        number_len += 1;                                    //当长度为奇数时，需要把后面的F也读出来
    oada->number = hexstr2number(ptr, number_len);          //1字节=2字符
    if (oada->number == RT_NULL)
    {
        ret = RT_EINVAL;
        goto __exit;
    }
    LOG_D("oada->number=>%s", oada->number);

    __exit:
//错误处理
    return ret;

}

/**
 * 解析PID参数
 * @param data  PID参数地址
 * @param pid   解析后的值
 * @return
 */
static rt_err_t get_PID_info(const char *data, PID_t *pid)
{
    rt_err_t ret = RT_EOK;
    char *ptr = data;
    hexstr2uint8_t(ptr, &pid->byte);
    LOG_D("pid->byte=>0x%x", pid->byte);
    return ret;
}

/**
 * 解析DCS参数
 * @param data  DCS参数地址
 * @param dcs   解析后的值
 * @return
 */
static rt_err_t get_DCS_info(const char *data, DCS_t *dcs)
{
    rt_err_t ret = RT_EOK;
    char *ptr = data;
    hexstr2uint8_t(ptr, &dcs->byte);
    LOG_D("dcs->byte=>0x%x", dcs->byte);
    return ret;
}

/**
 * 解析SCTS参数
 * @param data  SCTS参数地址
 * @param scts  解析后的值
 * @return
 */
static rt_err_t get_SCTS_info(const char *data, SCTS_VP_t *scts)
{
    rt_err_t ret = RT_EOK;
    char *ptr = data;
    decstr2uint8_t(ptr, &scts->year);
    ptr += 2;
    decstr2uint8_t(ptr, &scts->month);
    ptr += 2;
    decstr2uint8_t(ptr, &scts->day);
    ptr += 2;
    decstr2uint8_t(ptr, &scts->hour);
    ptr += 2;
    decstr2uint8_t(ptr, &scts->minute);
    ptr += 2;
    decstr2uint8_t(ptr, &scts->second);
    ptr += 2;
    decstr2uint8_t(ptr, &scts->timezone);
    LOG_D("scts=>%d-%d-%d %d:%d:%d +%d", scts->year, scts->month, scts->day, scts->hour, scts->minute, scts->second, scts->timezone);
    return ret;
}

/**
 * 解析数据长度及内容
 * @param data  UDL参数地址
 * @param udl   UDL的值
 * @return
 */
static rt_err_t get_UDL_info(const char *data, uint8_t *udl)
{
    rt_err_t ret = RT_EOK;
    char *ptr = data;
    hexstr2uint8_t(ptr, udl);
    LOG_D("udl=>%d", *udl);
    return ret;
}

/**
 * 解析数据内容
 * @param data  PDU格式数据指针首地址
 * @param dcs   协议类型
 * @param udl   数据长度
 * @param ud    返回的数据
 * @return  解析成功返回RT_EOK
 */
static rt_err_t get_UD_info(const char *data, DCS_t dcs, const uint8_t udl, char **ud)
{
    char *ptr = data;
    int length = strlen(ptr);
    LOG_D("ud=>%s", ptr);
    char *hexstr = RT_NULL;    //先把ascii格式的data转成hex
    hexstr = rt_malloc(length + 1);
    hexstr[length] = '\0';
    for (int i = 0; i < length; i++)
    {
        hexstr2uint8_t(ptr, hexstr + i);
        ptr += 2;
    }
    //
    LOG_HEX("hexstr", 16, hexstr, strlen(hexstr));

    switch (dcs.dcs_t.Encode)
    {
    case 0: //7bit
        *ud = rt_calloc(udl, sizeof(char));
        bit7str2uint8_tstr(hexstr, *ud, udl);
        LOG_HEX("7bit", 16, *ud, strlen(*ud));
        break;
    case 2: //USC2
        *ud = rt_calloc(udl, sizeof(char));
        /*以下两种情况二选一*/
        /*1.针对某些平台下发的短信会有中文的情况，直接过滤掉其中的中文，得到纯英文的指令*/
        usc2str2uint8_tstr(hexstr, *ud, udl);
        /*2./如果用户需要自行处理usc2编码的中文，这里直接memcpy，待用户自行处理*/
        //memcpy(*ud, hexstr, udl);
        LOG_HEX("usc2", 16, *ud, strlen(*ud));
        break;
    default:
        break;
    }
    rt_free(hexstr);
    return 0;
}

/**
 * 号码转hex字符串
 * @param dsc   转换后存储地址
 * @param src   number地址
 * @return
 */
static rt_err_t number2hexstr(char *dsc, const char *src)
{
    int length = strlen(src);
    int index = 0;
    for (; index < length; index += 2)
    {
        dsc[index] = src[index + 1];
        dsc[index + 1] = src[index];
    }
    if (length % 2) //长度为奇数，只需要补F
    {
        dsc[length - 1] = 'F';
    }
    return RT_EOK;
}

/**
 * 创建SCA字符串
 * @param data  存放SCA字符串的地址
 * @param sca   SCA参数
 * @return
 */
static rt_err_t create_SCA_info(char* data, SCA_t sca)
{
    char *ptr = data;
    sprintf(ptr, "%02X", sca.len);
    ptr += 2;
    sprintf(ptr, "%02X", sca.type);
    ptr += 2;
    number2hexstr(ptr, sca.number);
}

/**
 * 创建DA字符串
 * @param data  存放DA字符串的地址
 * @param da   DA参数
 * @return
 */
static rt_err_t create_DA_info(char* data, OA_DA_t da)
{
    char *ptr = data;
    sprintf(ptr, "%02X", da.len);
    ptr += 2;
    sprintf(ptr, "%02X", da.type);
    ptr += 2;
    number2hexstr(ptr, da.number);
}

/**
 * 创建VP字符串
 * @param dst   目标地址
 * @param scts  有效期时钟结构体
 * @return
 */
static rt_err_t create_VPabs_info(char* dst, SCTS_VP_t scts)
{
    char *ptr = dst;
    sprintf(ptr, "%d%d", scts.year % 10, scts.year / 10);
    ptr += 2;
    sprintf(ptr, "%d%d", scts.month % 10, scts.month / 10);
    ptr += 2;
    sprintf(ptr, "%d%d", scts.day % 10, scts.day / 10);
    ptr += 2;
    sprintf(ptr, "%d%d", scts.hour % 10, scts.hour / 10);
    ptr += 2;
    sprintf(ptr, "%d%d", scts.minute % 10, scts.minute / 10);
    ptr += 2;
    sprintf(ptr, "%d%d", scts.second % 10, scts.second / 10);
    ptr += 2;
    sprintf(ptr, "%d%d", scts.timezone % 10, scts.timezone / 10);
    ptr += 2;
}

/**
 *   创建VP字符串，相对时间
 * @param dst           存放地址
 * @param per5minute    多少分钟
 * @return
 */
static rt_err_t create_VPrel_info(char *dst, uint32_t minute)
{
    char *ptr = dst;
    uint8_t value = 0;
    if (minute <= 720)          //144*5min=12小时
        value = minute / 5 - 1;             //(vp+1)*5min
    else if (minute <= 1440)     //24小时
        value = (minute - 720) / 30 + 143;  //12hour+(vp-143)*30min
    else if (minute <= 43200)    //30天
        value = (minute / 1440 + 166);      //(vp-166)*1day
    else if (minute <= 635040)
        value = (minute / 10080) + 192;     //(vp-192)*1week
    else
        LOG_E("time overflow");             //相对值错误
    sprintf(ptr, "%2X", value);
}

/**
 * 创建UserData字符串
 * @param dst   目标地址
 * @param dcs   dcs参数，保存了编码
 * @param udl   数据长度
 * @param ud    存放数据的首地址
 * @return
 */
static rt_err_t create_UD_info(char *dst, DCS_t dcs, uint8_t datalength, char *userdata)
{
    int udl = datalength;                 //编码压缩后的实际长度，也就是实际udl,这里先等于datalength，如果用户处理纯中文
    char *hexstr = RT_NULL;
    hexstr = rt_calloc(256, sizeof(char));
    switch (dcs.dcs_t.Encode)
    {
    case 0: //7bit
        datalength = datalength > 140 ? 140 : datalength; //7bit模式下，短信长度不超过140
        udl = uint8_tstr2bit7str(userdata, hexstr, datalength);
        break;
    case 2: //USC2
        datalength = datalength > 70 ? 70 : datalength; //usc2模式下，短信长度不超过70
        /*以下2种情况二选一*/
        /*1.默认发送的字符串是纯英文*/
        udl = uint8_tstr2usc2str(userdata, hexstr, datalength);
        /*2.如果是USC2编码带中文，这里直接memcpy*/
        //memcpy(dst, userdata, datalength);
        break;
    default:
        break;
    }
    for (int i = 0; i < udl; i++)
    {
        sprintf(dst, "%02X", hexstr[i]);
        dst += 2;
    }

    return RT_EOK;
}

/**
 * 释放sms_data_t指针及成员指针指向的空间
 * @param sms
 */
void sms_submit_free(SMS_Submit_t *sms)
{
    rt_free(sms->sca.number);
    rt_free(sms->da.number);
    rt_free(sms->user_data);
    rt_free(sms);
}

/**
 * 释放sms_data_t指针及成员指针指向的空间
 * @param sms
 */
void sms_deliver_free(SMS_Deliver_t *sms)
{
    rt_free(sms->sca.number);
    rt_free(sms->oa.number);
    rt_free(sms->user_data);
    rt_free(sms);
}

/**
 * 解析接收方PDU格式数据
 * @param data      来自短信模块的数据
 * @return  解析后的sms_data_t结构体指针
 */
SMS_Deliver_t* sms_deliver_parse(const char *data)
{
    LOG_D("%s", data);
    uint8_t err_code = RT_EOK;
    SMS_Deliver_t *sms = RT_NULL;
    sms = rt_calloc(1, sizeof(SMS_Deliver_t));
    if (sms == RT_NULL)
    {
        err_code = 1;
        goto __exit;
    }

    if (get_SCA_info(data, &sms->sca) != RT_EOK)
    {
        err_code = 2;
        goto __exit;
    }

    data += (sms->sca.len + 1) * 2;
    if (get_PDUType_info(data, &sms->pdu) != RT_EOK)
    {
        err_code = 3;
        goto __exit;
    }

    data += 2;
    if (get_OADA_info(data, &sms->oa) != RT_EOK)
    {
        err_code = 4;
        goto __exit;
    }

    data += 4;
    data += (sms->oa.len % 2) ? (sms->oa.len + 1) : sms->oa.len;    //如果为奇数需要考虑F
    if (get_PID_info(data, &sms->pid) != RT_EOK)
    {
        err_code = 5;
        goto __exit;
    }

    data += 2;
    if (get_DCS_info(data, &sms->dcs) != RT_EOK)
    {
        err_code = 6;
        goto __exit;
    }

    data += 2;
    if (get_SCTS_info(data, &sms->scts) != RT_EOK)
    {
        err_code = 7;
        goto __exit;
    }

    data += 14;
    if (get_UDL_info(data, &sms->user_data_length) != RT_EOK)
    {
        err_code = 8;
        goto __exit;
    }

    data += 2;
    if (get_UD_info(data, sms->dcs, sms->user_data_length, &sms->user_data) != RT_EOK)
    {
        err_code = 9;
        goto __exit;
    }

    LOG_D("parse ok\n");
    __exit:
//如果没有错误直接返回，否则返回RT_NULL
    if (err_code == RT_EOK)
        return sms;

    LOG_E("sms parse failed,error code %d", err_code);
    sms_deliver_free(sms);
    return RT_NULL;
}

/**
 * 创建发送方PDU格式数据
 * @param sms   存放数据的结构体
 * @return  返回创建的字符串
 */
char *sms_submit_create(SMS_Submit_t sms)
{
    char *sms_str = RT_NULL;
    char *ptr = RT_NULL;

    sms_str = rt_calloc(256, sizeof(char));

    ptr = sms_str;
//    create_SCA_info(ptr, sms.sca);
    sprintf(ptr, "%02X", 0);            //发送时，sca可以直接写0，硬件会自动读取短信中心地址
    ptr += strlen(ptr);
    sprintf(ptr, "%02X", sms.pdu.byte);
    ptr += 2;
    sprintf(ptr, "%02X", sms.mr.byte);
    ptr += 2;
    create_DA_info(ptr, sms.da);
    ptr += strlen(ptr);
    sprintf(ptr, "%02X", sms.pid.byte);
    ptr += 2;
    sprintf(ptr, "%02X", sms.dcs.byte);
    ptr += 2;
    if (sms.pdu.submit.VPF == 2)        //02相对的
    {
        create_VPrel_info(ptr, sms.vp.rel);
        ptr += 2;
    }
    else if (sms.pdu.submit.VPF == 3)   //03:绝对的
    {
        create_VPabs_info(ptr, sms.vp.abs);
        ptr += strlen(ptr);
    }                                   //00-没有VP段，01-保留。这里均不作处理
    sprintf(ptr, "%02X", sms.user_data_length);
    ptr += 2;
    create_UD_info(ptr, sms.dcs, sms.user_data_length, sms.user_data);
    return sms_str;

}

/**
 * 快速创建一个英文内容的发送PDU对象
 * @param da_number 对方电话号码
 * @param user_data 字符串内容
 * @return  返回对象
 */
SMS_Submit_t* sms_submit_create_english_object(const char *da_number, const char *user_data)
{
    char sca_number[] = "8613010811500";
    //8613010811500为四川联通短信中心号码
    SMS_Submit_t *sms_sub = RT_NULL;
    sms_sub = rt_calloc(1, sizeof(SMS_Submit_t));
    sms_sub->sca.len = 8;
    sms_sub->sca.type = 0x91;
    sms_sub->sca.number = rt_calloc(strlen(sca_number) + 1, sizeof(char));
    memcpy(sms_sub->sca.number, sca_number, strlen(sca_number));

    sms_sub->pdu.byte = 0x31;
    sms_sub->mr.byte = 0;

    sms_sub->da.len = strlen(da_number);
    sms_sub->da.type = 0x91;
    sms_sub->da.number = rt_calloc(strlen(da_number) + 1, sizeof(char));
    memcpy(sms_sub->da.number, da_number, strlen(da_number));
    sms_sub->pid.byte = 0;
    sms_sub->dcs.byte = 0;  //英文

    sms_sub->vp.rel = 1440;
    sms_sub->user_data_length = strlen(user_data);
    sms_sub->user_data = rt_calloc(strlen(user_data) + 1, sizeof(char));
    memcpy(sms_sub->user_data, user_data, strlen(user_data));
    return sms_sub;
}

/**
 * 快速创建一个中文内容的发送PDU对象
 * @param da_number 对方电话号码
 * @param user_data 字符串内容
 * @return
 */
SMS_Submit_t* sms_submit_create_chinese_object(const char *da_number, const char *user_data)
{
    char sca_number[] = "8613010811500";
    //8613010811500为四川联通短信中心号码
    SMS_Submit_t *sms_sub = RT_NULL;
    sms_sub = rt_calloc(1, sizeof(SMS_Submit_t));
    sms_sub->sca.len = 8;
    sms_sub->sca.type = 0x91;
    sms_sub->sca.number = rt_calloc(strlen(sca_number) + 1, sizeof(char));
    memcpy(sms_sub->sca.number, sca_number, strlen(sca_number));

    sms_sub->pdu.byte = 0x31;
    sms_sub->mr.byte = 0;

    sms_sub->da.len = strlen(da_number);
    sms_sub->da.type = 0x91;
    sms_sub->da.number = rt_calloc(strlen(da_number) + 1, sizeof(char));
    memcpy(sms_sub->da.number, da_number, strlen(da_number));
    sms_sub->pid.byte = 0;
    sms_sub->dcs.byte = 0x08;   //中文

    sms_sub->vp.rel = 1440;
    sms_sub->user_data_length = strlen(user_data);
    sms_sub->user_data = rt_calloc(strlen(user_data) + 1, sizeof(char));
    memcpy(sms_sub->user_data, user_data, strlen(user_data));
    return sms_sub;
}
