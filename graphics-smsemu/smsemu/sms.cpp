#include <stdint.h>


#include "sms.h"
#include "internal.h"

#include <assert.h>
#include <string.h>


uint8_t smsCYC_00[256];//		(duunmap_types[0] + 0)
uint8_t smsCYC_ED[256];//		(duunmap_types[0] + 256 )			// 256b
uint8_t smsCYC_DDFD[256];//		(duunmap_types[0] + 256 + 256) 		// 256b
uint8_t smsCYC_CB[256];//		(duunmap_types[0] + 256 + 256 + 256) // 256b

uint8_t smsCARTRam0[16384];// 	(textures[0] + (16 * 1024))	// 16KB
uint8_t smsCARTRam1[16384];// 	(textures[0] + (32 * 1024))	// 16KB
uint8_t smsSysRamBuff[16384];// 	(textures[0] + (48 * 1024))	// 16KB




uint8_t *BIOS_ROM ;//= (uint8_t)BIOS_ROM_ALEXKID;	// 128k


static const bool valid_rom_size_values[0x10] =
{
    true,  // 0x0
    true,  // 0x1
    true,  // 0x2
    true,  // 0x3
    true,  // 0x4
    false, // 0x5
    false, // 0x6
    false, // 0x7
    false, // 0x8
    false, // 0x9
    false, // 0xA
    false, // 0xB
    true,  // 0xC
    false, // 0xD
    true,  // 0xE
    true   // 0xF
};

static const char* const valid_rom_size_string[0x10] =
{
    "256KiB",  // 0x0
    "512KiB",  // 0x1
    "1 MiB",   // 0x2
    "2 MiB",   // 0x3
    "4 MiB",   // 0x4
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, // 0x5..0xB
    "32KiB",   // 0xC
    nullptr,   // 0xD
    "64KiB",   // 0xE
    "128KiB"   // 0xF
};

static const char* const region_code_string[0x10] =
{
    "Unknown", // 0x0
    "Unknown", // 0x1
    "Unknown", // 0x2
    "SMS Japan",        // 0x3
    "SMS Export",       // 0x4
    "GG Japan",         // 0x5
    "GG Export",        // 0x6
    "GG International", // 0x7
    "Unknown", "Unknown", "Unknown", "Unknown", // 0x8..0xB
    "Unknown", "Unknown", "Unknown", "Unknown"  // 0xC..0xF
};





static uint32_t find_rom_header_offset(const uint8_t* data, uint32_t rom_size){
#if(0)
    {
        static const uint32_t header_offsets[] = {
        0x7FF0,  // 32KB boundary
        0x3FF0,  // 256KB boundary
        0x1FF0,  // 128KB boundary
        0xFFFF0, // 1MB boundary (for larger carts)
    };

    for (uint32_t i = 0; i < 4; ++i) {
        if (header_offsets[i] + 8 > rom_size) {
            continue; // skip out-of-bounds
        }
        if (memcmp(data + header_offsets[i], "TMR SEGA", 8) == 0) {
            dbug("\r\n   *TRM SEGA - FOUND:\r\n\r\n");
            return (uint16_t)header_offsets[i];
        }
    }

    // Not found
    return 0;
#endif
    static const uint8_t magic[] = {
            'T',
            'M',
            'R',
            ' ',
            'S',
            'E',
            'G',
            'A'
    };

    // 1. Expected location (standard for valid ROMs)
    if (rom_size >= 0x10) {
        size_t expected_offset = rom_size - 0x10;
        if (memcmp(&data[expected_offset], magic, 8) == 0)
            return expected_offset;
    }

    // 2. Scan backwards in 0x2000 chunks (used by BIOS)
    for (size_t offset = rom_size - 0x10; offset >= 0x10 && offset < rom_size; offset -= 0x2000) {
        if (memcmp(&data[offset], magic, 8) == 0)
            return (uint32_t)offset;
        if (offset < 0x2000) break; // Prevent underflow
    }

    // 3. Final fallback: scan full ROM (slow but exhaustive)
    for (size_t offset = 0; offset <= rom_size - 8; ++offset) {
        if (memcmp(&data[offset], magic, 8) == 0)
            return (uint32_t)offset;
    }

    // Not found
    return 0;

}

static uint16_t find_rom_header_offset_old(const uint8_t* data, int rom_size)
{
#if(0)
    // loop until we find the magic num
    // the rom header can start at 1 of 3 offsets
    static const uint16_t offsets[] =
    {
        // the bios checks in reverse order
        0x7FF0,
        0x3FF0,
        0x1FF0,
        0xFFFF0
    };

    for (size_t i = 0; i < ARRAY_SIZE(offsets); ++i)
    {
        const uint8_t* d = data + offsets[i];
        const char* magic = "TMR SEGA";

        if (d[0] == magic[0] && d[1] == magic[1] &&
            d[2] == magic[2] && d[3] == magic[3] &&
            d[4] == magic[4] && d[5] == magic[5] &&
            d[6] == magic[6] && d[7] == magic[7])
        {
            return offsets[i];
        }
    }
#endif
    // invalid offset, this zero needs to be checked by the caller!
    return 0;
}

uint32_t SMS_crc32(uint32_t crc, const void* data, size_t size)
{
    // SOURCE: http://home.thep.lu.se/~bjorn/crc/
    static const uint32_t CRC32_TABLE[0x100] =
    {
        0xD202EF8D, 0xA505DF1B, 0x3C0C8EA1, 0x4B0BBE37, 0xD56F2B94, 0xA2681B02, 0x3B614AB8, 0x4C667A2E, 0xDCD967BF, 0xABDE5729, 0x32D70693, 0x45D03605, 0xDBB4A3A6, 0xACB39330, 0x35BAC28A, 0x42BDF21C,
        0xCFB5FFE9, 0xB8B2CF7F, 0x21BB9EC5, 0x56BCAE53, 0xC8D83BF0, 0xBFDF0B66, 0x26D65ADC, 0x51D16A4A, 0xC16E77DB, 0xB669474D, 0x2F6016F7, 0x58672661, 0xC603B3C2, 0xB1048354, 0x280DD2EE, 0x5F0AE278,
        0xE96CCF45, 0x9E6BFFD3, 0x0762AE69, 0x70659EFF, 0xEE010B5C, 0x99063BCA, 0x000F6A70, 0x77085AE6, 0xE7B74777, 0x90B077E1, 0x09B9265B, 0x7EBE16CD, 0xE0DA836E, 0x97DDB3F8, 0x0ED4E242, 0x79D3D2D4,
        0xF4DBDF21, 0x83DCEFB7, 0x1AD5BE0D, 0x6DD28E9B, 0xF3B61B38, 0x84B12BAE, 0x1DB87A14, 0x6ABF4A82, 0xFA005713, 0x8D076785, 0x140E363F, 0x630906A9, 0xFD6D930A, 0x8A6AA39C, 0x1363F226, 0x6464C2B0,
        0xA4DEAE1D, 0xD3D99E8B, 0x4AD0CF31, 0x3DD7FFA7, 0xA3B36A04, 0xD4B45A92, 0x4DBD0B28, 0x3ABA3BBE, 0xAA05262F, 0xDD0216B9, 0x440B4703, 0x330C7795, 0xAD68E236, 0xDA6FD2A0, 0x4366831A, 0x3461B38C,
        0xB969BE79, 0xCE6E8EEF, 0x5767DF55, 0x2060EFC3, 0xBE047A60, 0xC9034AF6, 0x500A1B4C, 0x270D2BDA, 0xB7B2364B, 0xC0B506DD, 0x59BC5767, 0x2EBB67F1, 0xB0DFF252, 0xC7D8C2C4, 0x5ED1937E, 0x29D6A3E8,
        0x9FB08ED5, 0xE8B7BE43, 0x71BEEFF9, 0x06B9DF6F, 0x98DD4ACC, 0xEFDA7A5A, 0x76D32BE0, 0x01D41B76, 0x916B06E7, 0xE66C3671, 0x7F6567CB, 0x0862575D, 0x9606C2FE, 0xE101F268, 0x7808A3D2, 0x0F0F9344,
        0x82079EB1, 0xF500AE27, 0x6C09FF9D, 0x1B0ECF0B, 0x856A5AA8, 0xF26D6A3E, 0x6B643B84, 0x1C630B12, 0x8CDC1683, 0xFBDB2615, 0x62D277AF, 0x15D54739, 0x8BB1D29A, 0xFCB6E20C, 0x65BFB3B6, 0x12B88320,
        0x3FBA6CAD, 0x48BD5C3B, 0xD1B40D81, 0xA6B33D17, 0x38D7A8B4, 0x4FD09822, 0xD6D9C998, 0xA1DEF90E, 0x3161E49F, 0x4666D409, 0xDF6F85B3, 0xA868B525, 0x360C2086, 0x410B1010, 0xD80241AA, 0xAF05713C,
        0x220D7CC9, 0x550A4C5F, 0xCC031DE5, 0xBB042D73, 0x2560B8D0, 0x52678846, 0xCB6ED9FC, 0xBC69E96A, 0x2CD6F4FB, 0x5BD1C46D, 0xC2D895D7, 0xB5DFA541, 0x2BBB30E2, 0x5CBC0074, 0xC5B551CE, 0xB2B26158,
        0x04D44C65, 0x73D37CF3, 0xEADA2D49, 0x9DDD1DDF, 0x03B9887C, 0x74BEB8EA, 0xEDB7E950, 0x9AB0D9C6, 0x0A0FC457, 0x7D08F4C1, 0xE401A57B, 0x930695ED, 0x0D62004E, 0x7A6530D8, 0xE36C6162, 0x946B51F4,
        0x19635C01, 0x6E646C97, 0xF76D3D2D, 0x806A0DBB, 0x1E0E9818, 0x6909A88E, 0xF000F934, 0x8707C9A2, 0x17B8D433, 0x60BFE4A5, 0xF9B6B51F, 0x8EB18589, 0x10D5102A, 0x67D220BC, 0xFEDB7106, 0x89DC4190,
        0x49662D3D, 0x3E611DAB, 0xA7684C11, 0xD06F7C87, 0x4E0BE924, 0x390CD9B2, 0xA0058808, 0xD702B89E, 0x47BDA50F, 0x30BA9599, 0xA9B3C423, 0xDEB4F4B5, 0x40D06116, 0x37D75180, 0xAEDE003A, 0xD9D930AC,
        0x54D13D59, 0x23D60DCF, 0xBADF5C75, 0xCDD86CE3, 0x53BCF940, 0x24BBC9D6, 0xBDB2986C, 0xCAB5A8FA, 0x5A0AB56B, 0x2D0D85FD, 0xB404D447, 0xC303E4D1, 0x5D677172, 0x2A6041E4, 0xB369105E, 0xC46E20C8,
        0x72080DF5, 0x050F3D63, 0x9C066CD9, 0xEB015C4F, 0x7565C9EC, 0x0262F97A, 0x9B6BA8C0, 0xEC6C9856, 0x7CD385C7, 0x0BD4B551, 0x92DDE4EB, 0xE5DAD47D, 0x7BBE41DE, 0x0CB97148, 0x95B020F2, 0xE2B71064,
        0x6FBF1D91, 0x18B82D07, 0x81B17CBD, 0xF6B64C2B, 0x68D2D988, 0x1FD5E91E, 0x86DCB8A4, 0xF1DB8832, 0x616495A3, 0x1663A535, 0x8F6AF48F, 0xF86DC419, 0x660951BA, 0x110E612C, 0x88073096, 0xFF000000,
    };

    const uint8_t* u8_data = (const uint8_t*)data;

    for (size_t i = 0; i < size; ++i)
    {
        crc = CRC32_TABLE[(uint8_t)crc ^ u8_data[i]] ^ (crc >> 8);
    }

    return crc;
}

void SMS_set_system_type(struct SMS_Core* sms, enum SMS_System system)
{
    sms->system = system;
}

enum SMS_System SMS_get_system_type(const struct SMS_Core* sms)
{
    return sms->system;
}

bool SMS_is_system_type_gg(const struct SMS_Core* sms)
{
    //return 1;
    return SMS_get_system_type(sms) == SMS_System_GG;
}

void SMS_set_overscan_enable(struct SMS_Core* sms, bool enable)
{
    sms->overscan_enable = enable;
}

bool SMS_is_overscan_enabled(const struct SMS_Core* sms)
{
    return sms->overscan_enable;
}

bool SMS_is_spiderman_int_hack_enabled(const struct SMS_Core* sms)
{
    return sms->crc == 0xEBE45388;
}

struct SMS_RomHeader SMS_parse_rom_header(const uint8_t* data, uint16_t offset)
{
    struct SMS_RomHeader header = {0};

    memcpy(&header.magic, data + offset, sizeof(header.magic));
    // skip 2 padding bytes as well
    offset += sizeof(header.magic) + 2;

    memcpy(&header.checksum, data + offset, sizeof(header.checksum));
    offset += sizeof(header.checksum);

    // the next part depends on if the host is LE or BE.
    // due to needing to read half nibble.
    // for now, assume LE, as it likely will be...
    uint32_t last_4;
    memcpy(&last_4, data + offset, sizeof(last_4));

    header.prod_code = 0; // this isn't correct atm
    header.version = (last_4 >> 16) & 0xF;
    header.region_code = (last_4 >> 28) & 0xF;
    header.rom_size = (last_4 >> 24) & 0xF;

    return header;
}

static void log_header(const struct SMS_RomHeader* header)
{
    (void)header;
    dbug("\r\n\r\nHEADER:\r\n");
    dbug("version: [0x%X]\r\n", header->version);
    dbug("region_code: [0x%X] [%s]\r\n", header->region_code, region_code_string[header->region_code]);
    dbug("rom_size: [0x%X] [%s]\r\n", header->rom_size, valid_rom_size_string[header->rom_size]);
}

static void setup_mapper(struct SMS_Core* sms)
{
    // this is where it would figure out which mapper
    // to use, for now, hardcode sega mapper (99% of games)
    sms->cart.mapper_type = MAPPER_TYPE_SEGA;
    sega_mapper_setup(sms);

    //(void)codemaster_mapper_setup; // silence warning
}



static const uint8_t parity8_lookupSRC[256] = {
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0
};

__ATTR_RAM_TC uint8_t parity8_lookup[256];


bool SMS_init(struct SMS_Core* sms)
{
    memset(sms, 0x00, sizeof(struct SMS_Core));
    memcpy(parity8_lookup, parity8_lookupSRC, 256);

    return true;
}

static void SMS_reset(struct SMS_Core* sms)
{
    z80_init(sms);
    SN76489_init(sms);
    vdp_init(sms);

    BIOS_ROM = (uint8_t *)BIOS_ROM_ALEXKID;	// 128k


    // port A/B are hi when a button is NOT pressed
    sms->port.a = 0xFF;
    sms->port.b = 0xFF;

    sms->port.gg_regs[0x0] = 0xC0;
    sms->port.gg_regs[0x1] = 0x7F;
    sms->port.gg_regs[0x2] = 0xFF;
    sms->port.gg_regs[0x3] = 0x00;
    sms->port.gg_regs[0x4] = 0xFF;
    sms->port.gg_regs[0x5] = 0x00;
    sms->port.gg_regs[0x6] = 0xFF;
}

bool SMS_loadrom(struct SMS_Core* sms, const uint8_t* rom, size_t size)
{
    assert(sms && rom && size);

    dbug("[INFO] SMS_loadrom() called with rom size: 0x%X\r\n", size);

    // try to find the header offset
    const uint32_t header_offset = find_rom_header_offset(rom, size);

    // no header found!
    if (header_offset == 0)
    {
        dbug("[ERROR] unable to find rom header, well isnt this SHIT - going to try anyway!\r\n");
        //return false;
    }

    dbug("[INFO] found header offset at: 0x%X\n", header_offset);

    struct SMS_RomHeader header = SMS_parse_rom_header(rom, header_offset);
    log_header(&header);

     //dbug("region_code: [0x%X] [%s]\r\n", header->region_code, region_code_string[header->region_code]);
    setSMSPlotter(0);
    if (header.region_code > 4) {    // game gear mode{
        setSMSPlotter(1);
        SMS_set_system_type(sms, SMS_System_GG);
    } else {
        SMS_set_system_type(sms, SMS_System_SMS1);
    }


    // check if the size is valid
    if (!valid_rom_size_values[header.rom_size])
    {
        dbug("[ERROR] invalid rom size in header! 0x%X\r\n", header.rom_size);
        //return false;
    }

    // save the rom, setup the size and mask
    sms->rom_size = size;
    sms->rom_mask = size - 1; // this works because size is always pow2
    sms->cart.max_bank_mask = (size / 0x4000) - 1;
    sms->crc = SMS_crc32(0, rom, size);

    dbug("crc32 0x%08X\r\n", sms->crc);

    // this assumes the game is always sega mapper
    // which (for testing at least), it always will be
    setup_mapper(sms);

    SMS_reset(sms);

    return true;
}


/*
void SMS_set_apu_callback(struct SMS_Core* sms, sms_apu_callback_t cb, void* user, uint32_t freq)
{
    sms->apu_callback = cb;
    //sms->apu_callback_user = user;
    sms->apu_callback_freq = freq;
}
*/

enum { STATE_MAGIC = 0x5E6A };
enum { STATE_VER = 1 };


bool SMS_parity16(uint16_t value) {
    /*
    // SOURCE: https://graphics.stanford.edu/~seander/bithacks.html#ParityParallel
    value ^= value >> 8; // 16-bit
    value ^= value >> 4; // 8-bit
    value &= 0xF;
    return !((0x6996 >> value) & 0x1);
    */
    value ^= value >> 8;
    value ^= value >> 4;
    value ^= value >> 2;
    value ^= value >> 1;
    return value & 1;
}


bool SMS_parity8(uint8_t value) {
    return(parity8_lookup[value]);
    // SOURCE: https://graphics.stanford.edu/~seander/bithacks.html#ParityParallel
    value ^= value >> 4; // 8-bit
    value &= 0xF;
    return !((0x6996 >> value) & 0x1);

}


uint16_t smsAudioCycs, smsAudioTicks;
void vdp_run_pal(struct SMS_Core *sms, uint8_t cycles);

void SMS_step(struct SMS_Core *sms)
{
    z80_run(sms);
    const bool is_pal = (sms->region == REGION_PAL);
    if(is_pal)
        vdp_run_pal(sms, sms->cpu.cycles);  // pal region renderer
    else
        vdp_run(sms, sms->cpu.cycles);      // ntsc region renderer


    // audio sync: scale similarly
    // SN76489_run(sms, vdp_cycles);
}


#ifdef __linux__

int proc_dpad_a_fire() {return 0;}
int proc_dpad_a_fire2(){return 0;}
int proc_dpad_a_up()   {return 0;}
int proc_dpad_a_down() {return 0;}
int proc_dpad_a_left() {return 0;}
int proc_dpad_a_right(){return 0;}


#define JOYSTICK_A_INPUT_FIRE   proc_dpad_a_fire()
#define JOYSTICK_A_INPUT_FIRE2  proc_dpad_a_fire2()
#define JOYSTICK_A_INPUT_UP     proc_dpad_a_up()
#define JOYSTICK_A_INPUT_DOWN   proc_dpad_a_down()
#define JOYSTICK_A_INPUT_LEFT   proc_dpad_a_left()
#define JOYSTICK_A_INPUT_RIGHT  proc_dpad_a_right()

#endif


void SMS_run_frame_cycles(struct SMS_Core* sms, size_t cycles)
{
    //uint8_t port_a_state = 0;

    //if (JOYSTICK_A_INPUT_FIRE)  port_a_state |= JOY1_A_BUTTON;
    //if (JOYSTICK_A_INPUT_FIRE2) port_a_state |= JOY1_B_BUTTON;
    //if (JOYSTICK_A_INPUT_UP)    port_a_state |= JOY1_UP_BUTTON;
    //if (JOYSTICK_A_INPUT_DOWN)  port_a_state |= JOY1_DOWN_BUTTON;
    //if (JOYSTICK_A_INPUT_LEFT)  port_a_state |= JOY1_LEFT_BUTTON;
    //if (JOYSTICK_A_INPUT_RIGHT) port_a_state |= JOY1_RIGHT_BUTTON;
    //SMS_set_port_a(sms, (enum SMS_PortA)(uint8_t)port_a_state);

    for (size_t i = 0; i < cycles; i += sms->cpu.cycles)
        SMS_step(sms);
}

void SMS_run_frame_delta(struct SMS_Core *sms, double delta)
{
    const size_t cycles = (size_t) (delta * (double) (CYCLES_PER_FRAME));

    SMS_run_frame_cycles(sms, cycles);
}



void SMS_run_frame(struct SMS_Core *sms)
{
    SMS_run_frame_cycles(sms, CYCLES_PER_FRAME);


    // clear the controller inputs after doing what we want.
    sms->port.a = 0xff;
    sms->port.b = 0xff;
    sms->port.gg_regs[0] = 0xff;
}
