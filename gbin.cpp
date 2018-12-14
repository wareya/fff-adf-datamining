
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <vector>
#include <unordered_map>
#include <string>


char scratch[4];

FILE * f;

size_t read_scratch()
{
    memset(scratch, 0, 4);
    return fread(scratch, 1, 4, f);
}
uint8_t read_u8()
{
    uint8_t r;
    fread(&r, 1, 1, f);
    return r;
}
int8_t read_i8()
{
    int8_t r;
    fread(&r, 1, 1, f);
    return r;
}
uint16_t read_u16()
{
    uint16_t r;
    fread(&r, 2, 1, f);
    return r;
}
int16_t read_i16()
{
    int16_t r;
    fread(&r, 2, 1, f);
    return r;
}
uint32_t read_u32()
{
    uint32_t r;
    fread(&r, 4, 1, f);
    return r;
}
int32_t read_i32()
{
    int32_t r;
    fread(&r, 4, 1, f);
    return r;
}
uint64_t read_u64()
{
    uint64_t r;
    fread(&r, 8, 1, f);
    return r;
}
int64_t read_i64()
{
    int64_t r;
    fread(&r, 8, 1, f);
    return r;
}

float read_f16()
{
    uint16_t temp;
    fread(&temp, 2, 1, f);
    
    uint32_t sign = (temp&0x8000)>>15;
    sign <<= 31;
    
    int32_t exp = (temp&0x7C00)>>10;
    exp -= 15;
    exp += 127;
    uint32_t u_exp = exp;
    u_exp <<= 23;
    
    uint32_t fraction = (temp&0x03FF);
    fraction <<= 13;
    
    uint32_t temp2 = sign | u_exp | fraction;
    
    float r;
    memcpy(&r, &temp2, 4);
    return r;
}

float read_f32()
{
    float r;
    fread(&r, 4, 1, f);
    return r;
}

std::string read_string()
{
    std::string r;
    auto c = fgetc(f);
    while(c > 0 and c < 0x100)
    {
        r.push_back(char(c));
        c = fgetc(f);
    }
    return r;
}

struct header_t
{
    char magic[3];
    char endian;
    uint16_t field_04; // always 1
    uint16_t field_06; // always 0
    uint32_t field_08; // always 16
    uint32_t field_0c; // always 4
    uint32_t flags;
    uint32_t struct_offset;
    uint32_t struct_count;
    uint32_t struct_stride;
    uint32_t types_count;
    uint32_t types_offset;
    uint32_t field_28; // ??
    uint32_t string_table_offset;
    uint32_t field_30; // 0 or 4
    uint32_t padding[3]; // always 0
};

struct type_t
{
    uint16_t type;
    uint16_t offset;
};

int main(int argc, char ** argv)
{
    if(argc < 2)
        return puts("usage: ./ism2 <filename.ism>"), 0;
    //if(argc == 3 && std::string(argv[2]) == std::string("-alt"))
    //    alt_types = true;
        
    
    f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END);
    //auto flen = ftell(f);
    fseek(f, -0x40, SEEK_CUR);
    
    header_t header;
    memset(&header, 0, 0x40);
    fread(&header, 0x40, 1, f);
    if(memcmp(&header, "GBNL", 4) != 0)
        return puts("error: not a valid little-endian GBIN file"), 0;
    
    //printf("#struct offset/count/size 0x%08X 0x%08X 0x%08X\n", header.struct_offset, header.struct_count, header.struct_stride);
    
    //printf("#type offset/count 0x%08X 0x%08X\n", header.types_offset, header.types_count);
    
    //printf("#string table offset 0x%08X\n", header.string_table_offset);
    
    fseek(f, header.types_offset, SEEK_SET);
    std::vector<type_t> types;
    for(uint32_t i = 0; i < header.types_count; i++)
    {
        type_t type;
        fread(&type, 4, 1, f);
        types.push_back(type);
    }
    
    /*
    for(auto & type : types)
        printf("0x%04X:%d\n", type.offset, type.type);
    */
    
    //printf("[");
    fseek(f, header.struct_offset, SEEK_SET);
    //auto first = true;
    for(uint32_t i = 0; i < header.struct_count; i++)
    {
        //if(!first)
        //    printf(",\n [");
        //else
        //    printf("[");
        //first = false;
        
        auto start = ftell(f);
        auto first = true;
        for(auto & type : types)
        {
            if(!first)
                printf("\t");
            //    printf(", ");
            first = false;
            
            fseek(f, start + type.offset, SEEK_SET);
            if(type.type == 0)
                printf("%d", read_u32());
            else if(type.type == 1)
                printf("%d", read_u8());
            else if(type.type == 2)
                printf("%d", read_u16());
            else if(type.type == 3)
                printf("%f", read_f32());
            else if(type.type == 5)
            {
                auto string_offset = read_u32();
                if(string_offset == 0xFFFFFFFF)
                    printf("\"\"");
                else
                {
                    //printf("(((reading string from 0x%08X", header.string_table_offset + string_offset);
                    if(header.string_table_offset == 0)
                        return printf("\n\nerror: db referenced a string but there is no string table\n(triggered at 0x%08X)", ftell(f)), 0;
                    fseek(f, header.string_table_offset + string_offset, SEEK_SET);
                    auto c = read_u8();
                    while(c != 0)
                    {
                        if(c == '\t')
                            printf("\\t");
                        else if(c == '\n')
                            printf("\\n");
                        else if(c == '\r')
                            printf("\\r");
                        else if(c == '\\')
                            printf("\\\\");
                        else if(c == '"')
                            printf("\"\"");
                        else
                            printf("%c", c);
                        c = read_u8();
                    }
                    // TODO seek and print, with escapes
                }
            }
            else if(type.type == 6)
            {
                printf("%d", read_u64());
            }
            else
                return printf("\n\nerror: unsupported datatype %d (0, 1, 2, 3, and 5 are supported)\n(triggered at 0x%08X)", type.type, ftell(f)), 0;
        }
        fseek(f, start + header.struct_stride, SEEK_SET);
        //printf("]");
        printf("\n");
    }
    //printf("]\n");
}
