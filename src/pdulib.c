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
 * hex�ַ���תuint8_t��������
 * @param addr      �ַ����׵�ַ
 * @param value     ת��������ֵ
 * @return  �ɹ�����RT_EOK
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
 * dec�ַ���תuint8_t��������,�����ڴ���SCTSʱ������
 * @param addr      �ַ����׵�ַ
 * @param value     ת��������ֵ
 * @return  �ɹ�����RT_EOK
 */
static int decstr2uint8_t(const char *addr, uint8_t *value)
{
    uint8_t ret = RT_EOK;
    *value = (addr[0] - '0') + (addr[1] - '0') * 10;
    return ret;
}

/**
 * hex�ַ���ת�����ַ���
 * @param addr      �ַ����׵�ַ
 * @param length    �ַ�������
 * @return  ת����ĺ����ַ���ָ��
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
 * bit7ѹ����ʽ�ַ�����ԭ��ascii�ִ�
 * @param bit7str       �������ѹ���ַ���
 * @param uint8_tstr    �������ַ���
 * @param length        �������ѹ���ַ�������
 * @return  ����ת����ɵ�uint8_tstr�ִ�����
 */
static size_t bit7str2uint8_tstr(const char *bit7str, char *uint8_tstr, int length)
{
    uint8_t bit_pos = 0;

    uint8_t s7_pos = 0; //bit7str�ִ�����
    uint8_t s8_pos = 0; //uint8_tstr�ִ�����

    uint8_tstr[s8_pos] = bit7str[s7_pos] & 0x7F;

    for (bit_pos = 1, s8_pos = 1;; s8_pos++, bit_pos++)
    {
        if (s7_pos >= length)
            break;

        if (bit_pos % 8 == 0)   //��8*n�β���Ҫ��λ
            bit_pos = 0;
        else
            s7_pos++;           //����ʱ���ǿ�byte��

        uint8_t big = (bit7str[s7_pos] << bit_pos);
        uint8_t small = (bit7str[s7_pos - 1] >> (8 - bit_pos));
        uint8_tstr[s8_pos] = (big | small) & 0x7F;
        //���������϶�ʱ���˴�ʹ��logd���������Ϣ�Ῠ��
        //rt_kprintf("s7_pos=>%d,bit_pos=>%d,big=>%x,small=>%x,uint8_tstr[%d]=>%x\n", s7_pos, bit_pos, big, small, s8_pos, uint8_tstr[s8_pos]);
    }
    return s8_pos;
}

/**
 * usc2�ַ����������ģ���ԭ��ascii�ִ�
 * @param usc2str       ��������ַ���
 * @param uint8_tstr    ��������ַ���
 * @param length        �������ַ�������
 * @return  ����ת����ɵ�uint8_tstr�ִ�����
 */
static size_t usc2str2uint8_tstr(const char *usc2str, char *uint8_tstr, int length)
{
    uint8_t s2_pos = 0; //usc2str�ִ�����
    uint8_t s8_pos = 0; //uint8_tstr�ִ�����
    for (; s2_pos < length; s2_pos = s2_pos + 2)
    {
        if (usc2str[s2_pos] != 0)  //��λ����0������
            continue;
        uint8_tstr[s8_pos] = usc2str[s2_pos + 1];
        s8_pos++;
    }
    return s2_pos;
}

/**
 * ascii�ִ�ѹ����bit7��ʽ�ַ���
 * @param uint8_tstr    ��ѹ�����ַ����׵�ַ
 * @param bit7str       ѹ������ַ����׵�ַ
 * @param length        ��ѹ�����ַ�������
 * @return  ѹ����ĳ���
 */
static size_t uint8_tstr2bit7str(const char *uint8_tstr, char *bit7str, int length)
{
    uint8_t bit_pos = 0;

    uint8_t s7_pos = 0; //bit7str�ִ�����
    uint8_t s8_pos = 0; //uint8_tstr�ִ�����
    LOG_D("using bit7 encoding...");
    for (;; s8_pos++, bit_pos++)
    {
        if (s8_pos >= length)
            break;

        if (bit_pos % 7 == 0)   //��7*n�β���Ҫ��λ
            bit_pos = 0;
        else
            s7_pos++;           //����ʱ���ǿ�byte��

        uint8_t small = uint8_tstr[s8_pos] >> bit_pos;
        uint8_t big = uint8_tstr[s8_pos + 1] << (7 - bit_pos);
        bit7str[s7_pos] = small | big;

        //���������϶�ʱ���˴�ʹ��logd���������Ϣ�Ῠ��
        //rt_kprintf("s7_pos=>%d,bit_pos=>%d,big=>%x,small=>%x,uint8_tstr[%d]=>%x\n", s7_pos, bit_pos, big, small, s8_pos, uint8_tstr[s8_pos]);
    }
    return s7_pos;
}

/**
 * ascii�ִ�תusc2
 * @param uint8_tstr    ��������ַ����׵�ַ
 * @param usc2str       �������ַ����׵�ַ
 * @param length        ��������ַ�������
 * @return  �������ַ�������
 */
static size_t uint8_tstr2usc2str(const char *uint8_tstr, char *usc2str, int length)
{
    uint8_t s2_pos = 0; //usc2str�ִ�����
    uint8_t s8_pos = 0; //uint8_tstr�ִ�����
    LOG_D("using usc2 encoding...");
    for (; s8_pos < length; s8_pos++, s2_pos += 2)
    {
        usc2str[s2_pos] = 0;
        usc2str[s2_pos + 1] = uint8_tstr[s8_pos];
    }
    return s2_pos;
}

/**
 * ����SCA����Ϣ���ĵ�ַ��Ϣ
 * @param data  �����������׵�ַ
 * @param sca   �������
 * @return �����ɹ�����RT_EOK
 */
static rt_err_t get_SCA_info(const char *data, SCA_t *sca)
{
    rt_err_t ret = RT_EOK;
    uint8_t number_len = 0;
    char *ptr = data;

    hexstr2uint8_t(ptr, &sca->len);
    if (sca->len != 8)                //ͨ������³���Ϊ8����������Ȳ�����
    {
        ret = RT_EINVAL;
        goto __exit;
    }
    LOG_D("sca->len=>%d", sca->len);

    ptr += 2;
    hexstr2uint8_t(ptr, &sca->type);
    if (sca->type & 0x80 == 0)    //���λӦ��Ϊ1
    {
        //bit6-bit4:��ֵ���ͣ�Type of Number����000��δ֪��001�����ʣ�010������,111��������չ��
        //bit3-bit0:�������Numbering plan identification��:0000��δ֪��0001��ISDN/�绰����(E.164/E.163)��1111��������չ��
        ret = RT_EINVAL;
        goto __exit;
    }
    LOG_D("sca->type=>0x%x", sca->type);

    ptr += 2;
    number_len = sca->len - 1;                              //����Ϣ���ĺ���=����-���ͳ���(1�ֽ�)
    sca->number = hexstr2number(ptr, number_len * 2);       //1�ֽ�=2�ַ�
    if (sca->number == RT_NULL)
    {
        ret = RT_EINVAL;
        goto __exit;
    }
    LOG_D("sca->number=>%s", sca->number);

    __exit:
//������
    return ret;

}

/**
 * ����Э�����ݵ�Ԫ����
 * @param data  �����������׵�ַ
 * @param pdu   �������
 * @return �����ɹ�����RT_EOK
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
 * ����OA/DA���ͷ���ַ/���շ���ַ��Ϣ
 * @param data  �����������׵�ַ
 * @param oada  �������
 * @return �����ɹ�����RT_EOK
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
    if (oada->type & 0x80 == 0)    //���λӦ��Ϊ1
    {
        //bit6-bit4:��ֵ���ͣ�Type of Number����000��δ֪��001�����ʣ�010������,111��������չ��
        //bit3-bit0:�������Numbering plan identification��:0000��δ֪��0001��ISDN/�绰����(E.164/E.163)��1111��������չ��
        ret = RT_EINVAL;
        goto __exit;
    }
    LOG_D("oada->type=>0x%x", oada->type);

    ptr += 2;
    number_len = oada->len;                                 //����Ϣ���ĺ���=����-���ͳ���(1�ֽ�)
    if (number_len % 2)
        number_len += 1;                                    //������Ϊ����ʱ����Ҫ�Ѻ����FҲ������
    oada->number = hexstr2number(ptr, number_len);          //1�ֽ�=2�ַ�
    if (oada->number == RT_NULL)
    {
        ret = RT_EINVAL;
        goto __exit;
    }
    LOG_D("oada->number=>%s", oada->number);

    __exit:
//������
    return ret;

}

/**
 * ����PID����
 * @param data  PID������ַ
 * @param pid   �������ֵ
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
 * ����DCS����
 * @param data  DCS������ַ
 * @param dcs   �������ֵ
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
 * ����SCTS����
 * @param data  SCTS������ַ
 * @param scts  �������ֵ
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
 * �������ݳ��ȼ�����
 * @param data  UDL������ַ
 * @param udl   UDL��ֵ
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
 * ������������
 * @param data  PDU��ʽ����ָ���׵�ַ
 * @param dcs   Э������
 * @param udl   ���ݳ���
 * @param ud    ���ص�����
 * @return  �����ɹ�����RT_EOK
 */
static rt_err_t get_UD_info(const char *data, DCS_t dcs, const uint8_t udl, char **ud)
{
    char *ptr = data;
    int length = strlen(ptr);
    LOG_D("ud=>%s", ptr);
    char *hexstr = RT_NULL;    //�Ȱ�ascii��ʽ��dataת��hex
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
        /*�������������ѡһ*/
        /*1.���ĳЩƽ̨�·��Ķ��Ż������ĵ������ֱ�ӹ��˵����е����ģ��õ���Ӣ�ĵ�ָ��*/
        usc2str2uint8_tstr(hexstr, *ud, udl);
        /*2./����û���Ҫ���д���usc2��������ģ�����ֱ��memcpy�����û����д���*/
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
 * ����תhex�ַ���
 * @param dsc   ת����洢��ַ
 * @param src   number��ַ
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
    if (length % 2) //����Ϊ������ֻ��Ҫ��F
    {
        dsc[length - 1] = 'F';
    }
    return RT_EOK;
}

/**
 * ����SCA�ַ���
 * @param data  ���SCA�ַ����ĵ�ַ
 * @param sca   SCA����
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
 * ����DA�ַ���
 * @param data  ���DA�ַ����ĵ�ַ
 * @param da   DA����
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
 * ����VP�ַ���
 * @param dst   Ŀ���ַ
 * @param scts  ��Ч��ʱ�ӽṹ��
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
 *   ����VP�ַ��������ʱ��
 * @param dst           ��ŵ�ַ
 * @param per5minute    ���ٷ���
 * @return
 */
static rt_err_t create_VPrel_info(char *dst, uint32_t minute)
{
    char *ptr = dst;
    uint8_t value = 0;
    if (minute <= 720)          //144*5min=12Сʱ
        value = minute / 5 - 1;             //(vp+1)*5min
    else if (minute <= 1440)     //24Сʱ
        value = (minute - 720) / 30 + 143;  //12hour+(vp-143)*30min
    else if (minute <= 43200)    //30��
        value = (minute / 1440 + 166);      //(vp-166)*1day
    else if (minute <= 635040)
        value = (minute / 10080) + 192;     //(vp-192)*1week
    else
        LOG_E("time overflow");             //���ֵ����
    sprintf(ptr, "%2X", value);
}

/**
 * ����UserData�ַ���
 * @param dst   Ŀ���ַ
 * @param dcs   dcs�����������˱���
 * @param udl   ���ݳ���
 * @param ud    ������ݵ��׵�ַ
 * @return
 */
static rt_err_t create_UD_info(char *dst, DCS_t dcs, uint8_t datalength, char *userdata)
{
    int udl = datalength;                 //����ѹ�����ʵ�ʳ��ȣ�Ҳ����ʵ��udl,�����ȵ���datalength������û���������
    char *hexstr = RT_NULL;
    hexstr = rt_calloc(256, sizeof(char));
    switch (dcs.dcs_t.Encode)
    {
    case 0: //7bit
        datalength = datalength > 140 ? 140 : datalength; //7bitģʽ�£����ų��Ȳ�����140
        udl = uint8_tstr2bit7str(userdata, hexstr, datalength);
        break;
    case 2: //USC2
        datalength = datalength > 70 ? 70 : datalength; //usc2ģʽ�£����ų��Ȳ�����70
        /*����2�������ѡһ*/
        /*1.Ĭ�Ϸ��͵��ַ����Ǵ�Ӣ��*/
        udl = uint8_tstr2usc2str(userdata, hexstr, datalength);
        /*2.�����USC2��������ģ�����ֱ��memcpy*/
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
 * �ͷ�sms_data_tָ�뼰��Աָ��ָ��Ŀռ�
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
 * �ͷ�sms_data_tָ�뼰��Աָ��ָ��Ŀռ�
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
 * �������շ�PDU��ʽ����
 * @param data      ���Զ���ģ�������
 * @return  �������sms_data_t�ṹ��ָ��
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
    data += (sms->oa.len % 2) ? (sms->oa.len + 1) : sms->oa.len;    //���Ϊ������Ҫ����F
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
//���û�д���ֱ�ӷ��أ����򷵻�RT_NULL
    if (err_code == RT_EOK)
        return sms;

    LOG_E("sms parse failed,error code %d", err_code);
    sms_deliver_free(sms);
    return RT_NULL;
}

/**
 * �������ͷ�PDU��ʽ����
 * @param sms   ������ݵĽṹ��
 * @return  ���ش������ַ���
 */
char *sms_submit_create(SMS_Submit_t sms)
{
    char *sms_str = RT_NULL;
    char *ptr = RT_NULL;

    sms_str = rt_calloc(256, sizeof(char));

    ptr = sms_str;
//    create_SCA_info(ptr, sms.sca);
    sprintf(ptr, "%02X", 0);            //����ʱ��sca����ֱ��д0��Ӳ�����Զ���ȡ�������ĵ�ַ
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
    if (sms.pdu.submit.VPF == 2)        //02��Ե�
    {
        create_VPrel_info(ptr, sms.vp.rel);
        ptr += 2;
    }
    else if (sms.pdu.submit.VPF == 3)   //03:���Ե�
    {
        create_VPabs_info(ptr, sms.vp.abs);
        ptr += strlen(ptr);
    }                                   //00-û��VP�Σ�01-�������������������
    sprintf(ptr, "%02X", sms.user_data_length);
    ptr += 2;
    create_UD_info(ptr, sms.dcs, sms.user_data_length, sms.user_data);
    return sms_str;

}

/**
 * ���ٴ���һ��Ӣ�����ݵķ���PDU����
 * @param da_number �Է��绰����
 * @param user_data �ַ�������
 * @return  ���ض���
 */
SMS_Submit_t* sms_submit_create_english_object(const char *da_number, const char *user_data)
{
    char sca_number[] = "8613010811500";
    //8613010811500Ϊ�Ĵ���ͨ�������ĺ���
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
    sms_sub->dcs.byte = 0;  //Ӣ��

    sms_sub->vp.rel = 1440;
    sms_sub->user_data_length = strlen(user_data);
    sms_sub->user_data = rt_calloc(strlen(user_data) + 1, sizeof(char));
    memcpy(sms_sub->user_data, user_data, strlen(user_data));
    return sms_sub;
}

/**
 * ���ٴ���һ���������ݵķ���PDU����
 * @param da_number �Է��绰����
 * @param user_data �ַ�������
 * @return
 */
SMS_Submit_t* sms_submit_create_chinese_object(const char *da_number, const char *user_data)
{
    char sca_number[] = "8613010811500";
    //8613010811500Ϊ�Ĵ���ͨ�������ĺ���
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
    sms_sub->dcs.byte = 0x08;   //����

    sms_sub->vp.rel = 1440;
    sms_sub->user_data_length = strlen(user_data);
    sms_sub->user_data = rt_calloc(strlen(user_data) + 1, sizeof(char));
    memcpy(sms_sub->user_data, user_data, strlen(user_data));
    return sms_sub;
}
