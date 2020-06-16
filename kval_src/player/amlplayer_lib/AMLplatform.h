#ifndef AMLPLATFORM_H
#define AMLPLATFORM_H

#include <QObject>

#define EFUSE_INFO_GET      _IO('f', 0x40)

typedef struct __attribute__((__packed__)) aml_img_header {
    unsigned char magic[4]; //@AML
    uint32_t total_len;
    uint8_t header_len;
    uint8_t unk_x9;
    uint8_t unk_xA;
    uint8_t unk_xB;
    uint32_t unk_xC;
    uint32_t sig_type;
    uint32_t sig_offset;
    uint32_t sig_len;
    uint32_t data_offset;
    uint32_t unk_x20;
    uint32_t cert_offset;
    uint32_t cert_len;
    uint32_t data_len;
    uint32_t unk_x30;
    uint32_t code_offset;
    uint32_t code_len;
    uint32_t unk_x3C;
} aml_img_header_t;

struct efusekey_info {
    char keyname[32];
    unsigned int offset;
    unsigned int size;
};

enum AML_SUPPORT_H264_4K2K
{
  AML_SUPPORT_H264_4K2K_UNINIT = -1,
  AML_NO_H264_4K2K,
  AML_HAS_H264_4K2K,
  AML_HAS_H264_4K2K_SAME_PROFILE
};

bool aml_present();
bool aml_permissions();
bool aml_wired_present();
bool aml_support_hevc();
bool aml_support_hevc_4k2k();
bool aml_support_hevc_10bit();
AML_SUPPORT_H264_4K2K aml_support_h264_4k2k();
bool aml_support_vp9();
void aml_set_audio_passthrough(bool passthrough);
bool aml_get_usid(char * val);
bool aml_get_btl_uuid(char * filename, char * val);

#endif // AMLPLATFORM_H
