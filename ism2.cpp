// Very primitive ISM2 -> .obj converter.
// Only works well enough to dump FFF:ADF's map files with enough information to log where hidden treasures are located.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <vector>
#include <unordered_map>
#include <string>

bool swap_endian = false;

char scratch[4];

FILE * f;

size_t read_scratch()
{
    memset(scratch, 0, 4);
    return fread(scratch, 1, 4, f);
}

/*
void byteswap(char * addr, int n)
{
    char scratch[4];
}
*/

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

struct sec_data {
    uint32_t type;
    uint32_t offset;
};

struct vec2 {
    float x, y;
};
struct vec3 {
    float x, y, z;
};
struct vec4 {
    float x, y, z, w;
};
struct rgba {
    uint8_t r, g, b, a;
};

vec3 read_vec3()
{
    return vec3{read_f32(), read_f32(), read_f32()};
}
vec3 read_vec3_half()
{
    return vec3{read_f16(), read_f16(), read_f16()};
}
vec4 read_vec4()
{
    return vec4{read_f32(), read_f32(), read_f32(), read_f32()};
}
vec4 read_vec4_half()
{
    return vec4{read_f16(), read_f16(), read_f16(), read_f16()};
}
rgba read_rgba()
{
    return rgba{read_u8(), read_u8(), read_u8(), read_u8()};
}

struct vertex {
    vec3 pos;
    vec3 normal;
    vec3 normal2;
    vec2 uv;
    rgba color;
};

vertex read_vertex()
{
    vertex vert;
    vert.pos = read_vec3();
    vert.normal = read_vec3_half();
    auto u = read_f16();
    vert.normal2 = read_vec3_half();
    auto v = read_f16();
    vert.uv = vec2{u, v};
    vert.color  = read_rgba();
    //puts("read a vertex");
    //printf("y normal was: %f\n", vert.normal.y);
    return vert;
}

struct triangle {
    uint32_t a, b, c; // indexes into a vertex list
};

triangle read_triangle_16()
{
    return triangle{read_u16(), read_u16(), read_u16()};
}

triangle read_triangle_32()
{
    return triangle{read_u32(), read_u32(), read_u32()};
}

struct object {
    uint32_t offset;
    std::string name;
    vec3 pos;
    std::vector<uint8_t> raw;
};

int main(int argc, char ** argv)
{
    if(argc < 2)
        return puts("usage: ./ism2 <filename.ism>"), 0;
    f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END);
    auto flen = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    read_scratch();
    if(memcmp(scratch, "ISM2", 4) != 0)
        return puts("error: not an ism2 file"), 0;
    
    read_u32();
    read_u32();
    read_u32();
    
    // 0x10
    auto spec_flen = read_u32();
    if(spec_flen != uint32_t(flen))
        return puts("error: not little endian"), 0;
    
    auto section_count = read_u32();
    
    read_u32();
    read_u32();
    
    // 0x20
    
    std::vector<sec_data> sections;
    
    for(uint32_t i = 0; i < section_count; i++)
    {
        sections.push_back({read_u32(), read_u32()});
    }
    
    std::vector<std::string> strings;
    
    std::vector<vertex> vertices;
    std::vector<triangle> triangles;
    
    std::vector<object> objects;
    
    for(auto & section : sections)
    {
        printf("#section type %d at 0x%08X\n", section.type, section.offset);
        fseek(f, section.offset, SEEK_SET);
        switch(section.type)
        {
        case 33:
        {
            puts("# ### string table");
            read_u32();
            read_u32();
            
            auto string_count = read_u32();
            printf("#string count 0x%02X\n", string_count);
            for(uint32_t i = 0; i < string_count; i++)
                read_u32();
            for(uint32_t i = 0; i < string_count; i++)
            {
                strings.push_back(read_string());
                printf("#%s\n", strings.back().data());
            }
            break;
        }
        case 3:
        {
            puts("# ### node table");
            read_u32();
            read_u32();
            
            auto node_count = read_u32();
            printf("#node count 0x%02X\n", node_count);
            
            read_u32();
            read_u32();
            
            std::vector<uint32_t> node_offsets;
            for(uint32_t i = 0; i < node_count; i++)
                node_offsets.push_back(read_u32());
            
            for(auto & offset : node_offsets)
            {
                printf("#node offset 0x%08X\n", offset);
                fseek(f, offset, SEEK_SET);
                
                std::vector<uint8_t> metadata;
                for(uint32_t i = 0; i < 0x44; i++)
                    metadata.push_back(read_u8());
                
                fseek(f, offset, SEEK_SET);
                
                object myobj;
                
                myobj.offset = offset;
                
                // e.g. 0x670
                
                auto type = read_u32();
                printf("#node type %d\n", type);
                read_u32();
                auto subdata_count = read_u32();
                
                auto name = strings[read_u32()];
                myobj.name = name;
                
                printf("#node name %s\n", name.data());
                
                printf("#subdata count %d\n", subdata_count);
                
                // e.g. 0x680
                
                read_u32();
                read_u32();
                read_u32();
                read_u32();
                
                read_u32();
                read_u32();
                
                // e.g. 0x698
                
                read_u32();
                read_u32();
                
                // e.g. 0x6A0
                
                read_u32();
                auto boneid = read_u32();
                read_u32();
                read_u32();
                
                // e.g. 0x6B0
                
                std::vector<uint32_t> subdata_offsets;
                for(uint32_t i = 0; i < subdata_count; i++)
                    subdata_offsets.push_back(read_u32());
                
                for(auto & offset : subdata_offsets)
                {
                    fseek(f, offset, SEEK_SET);
                    printf("#subdata offset 0x%08X\n", offset);
                    auto type = read_u32();
                    printf("#subdata node type %d\n", type);
                    if(type == 91)
                    {
                        puts("#(transformation or metadata)");
                        read_u32();
                        auto transform_count = read_u32();
                        printf("#transform count %d\n", transform_count);
                        std::vector<uint32_t> transform_offsets;
                        for(uint32_t i = 0; i < transform_count; i++)
                            transform_offsets.push_back(read_u32());
                        for(auto & offset : transform_offsets)
                        {
                            fseek(f, offset, SEEK_SET);
                            auto type = read_u32();
                            printf("#transform type %d\n", type);
                            if(type == 20)
                            {
                                read_u32();
                                myobj.pos = read_vec3();
                            }
                        }
                    }
                }
                objects.push_back(myobj);
            }
            
            break;
        }
        case 11:
        {
            puts("# ### object table");
            read_u32();
            read_u32();
            
            auto object_block_count = read_u32();
            printf("#object block count 0x%02X\n", object_block_count);
            
            std::vector<uint32_t> object_block_offsets;
            
            for(uint32_t i = 0; i < object_block_count; i++)
                object_block_offsets.push_back(read_u32());
            
            for(auto & object_block_offset : object_block_offsets)
            {
                fseek(f, object_block_offset, SEEK_SET);
                
                auto obj_type = read_u32();
                printf("#object block type %d\n", obj_type);
                if(obj_type != 10)
                    return printf("#unknown/unsupported object block type %d", obj_type), 0;
                read_u32();
                
                auto geometry_block_count = read_u32();
                
                read_u32();
                read_u32();
                read_u32();
                read_u32();
                read_u32();
                
                std::vector<uint32_t> geometry_block_offsets;
                
                for(uint32_t i = 0; i < geometry_block_count; i++)
                    geometry_block_offsets.push_back(read_u32());
                
                for(auto & geometry_block_offset : geometry_block_offsets)
                {
                    fseek(f, geometry_block_offset, SEEK_SET);
                    
                    auto geo_type = read_u32();
                    switch(geo_type)
                    {
                    case 89:
                    {
                        puts("#vertex list");
                        read_u32();
                        
                        auto unk_count = read_u32(); // always 4?
                        auto vertex_type = read_u16();
                        
                        read_u16();
                        
                        auto vertex_count = read_u32();
                        auto vertex_size = read_u32();
                        read_u32();
                        
                        fseek(f, 4*unk_count, SEEK_CUR);
                        fseek(f, 24*unk_count, SEEK_CUR);
                        fseek(f, -4, SEEK_CUR); // we want last u32 of any of those unk entries, so we just go to the last unk entry and seek back
                        auto vertex_table_offset = read_u32();
                        
                        printf("#count 0x%08X offset 0x%08X\n", vertex_count, vertex_table_offset);
                        
                        fseek(f, vertex_table_offset, SEEK_SET);
                        
                        if(vertex_type == 1)
                        {
                            for(uint32_t i = 0; i < vertex_count; i++)
                                vertices.push_back(read_vertex());
                        }
                        
                        printf("#end of vertex list at 0x%08X\n", uint32_t(ftell(f)));
                        
                        break;
                    }
                    case 70:
                    {
                        puts("#polygon list");
                        read_u32();
                        read_u32();
                        read_u32();
                        read_u32();
                        
                        read_u16();
                        read_u16();
                        
                        auto poly_count = read_u32();
                        auto poly_start = read_u32();
                        auto poly_end = read_u32();
                        
                        auto poly_type = read_u32();
                        read_u32();
                        auto vert_count = read_u32();
                        //read_u32();
                        
                        auto index_type = read_u16();
                        printf("#index type %d\n", index_type);
                        read_u16();
                        read_u32();
                        
                        std::vector<uint32_t> polyoffsets;
                        
                        if(poly_type != 69)
                            return printf("#unknown poly type %d\n", poly_type), 0;
                        
                        printf("#vertex count %d (%d)\n", vert_count, vert_count/3*3);
                        
                        for(uint32_t i = 0; i < vert_count/3; i++)
                        {
                            if(index_type == 5)
                                triangles.push_back(read_triangle_16());
                            else if(index_type == 7)
                                triangles.push_back(read_triangle_32());
                        }
                        
                        break;
                    }
                    }
                }
            }
            break;
        }
        }
    }
    
    
    bool wavefront = true;
    if(wavefront)
    {
        printf("o World\n\n");
        
        uint32_t vert_index = 0;
        for(auto & vert : vertices)
            printf("#%d\nv %f %f %f %f %f %f\nvt %f %f\nvn %f %f %f\n",
                ++vert_index,
                vert.pos.x, vert.pos.y, vert.pos.z, vert.color.r/255.0, vert.color.g/255.0, vert.color.b/255.0,
                vert.uv.x, vert.uv.y,
                vert.normal.x, vert.normal.y, vert.normal.z);
        puts("");
        for(auto & tri : triangles)
            printf("f %d/%d/%d %d/%d/%d %d/%d/%d\n",
                tri.a+1, tri.a+1, tri.a+1,
                tri.b+1, tri.b+1, tri.b+1,
                tri.c+1, tri.c+1, tri.c+1);
        puts("");
        
        int i = 0;
        for(auto & object : objects)
        {
            float r = 0.5;
            printf("#offset 0x%08X\n", object.offset);
            printf("o Object%d_%s\n\n", i++, object.name.data());
            printf("v %f %f %f\n", object.pos.x-r, object.pos.y-r, object.pos.z-r);
            printf("v %f %f %f\n", object.pos.x-r, object.pos.y-r, object.pos.z+r);
            printf("v %f %f %f\n", object.pos.x-r, object.pos.y+r, object.pos.z-r);
            printf("v %f %f %f\n", object.pos.x-r, object.pos.y+r, object.pos.z+r);
            printf("v %f %f %f\n", object.pos.x+r, object.pos.y-r, object.pos.z-r);
            printf("v %f %f %f\n", object.pos.x+r, object.pos.y-r, object.pos.z+r);
            printf("v %f %f %f\n", object.pos.x+r, object.pos.y+r, object.pos.z-r);
            printf("v %f %f %f\n\n", object.pos.x+r, object.pos.y+r, object.pos.z+r);
            
            printf("f %d %d %d %d\n", vert_index+1, vert_index+2, vert_index+4, vert_index+3);
            printf("f %d %d %d %d\n", vert_index+5, vert_index+6, vert_index+8, vert_index+7);
            printf("f %d %d %d %d\n", vert_index+1, vert_index+2, vert_index+6, vert_index+5);
            printf("f %d %d %d %d\n", vert_index+3, vert_index+4, vert_index+8, vert_index+7);
            printf("f %d %d %d %d\n", vert_index+1, vert_index+3, vert_index+7, vert_index+5);
            printf("f %d %d %d %d\n\n", vert_index+2, vert_index+4, vert_index+8, vert_index+6);
            
            vert_index += 8;
        }
    }
    else
    {
        auto print_position = [&](auto pos)
        {
            printf("(%f, %f, %f)", pos.x, pos.y, pos.z);
        };
        auto print_vertex = [&](auto vert)
        {
            print_position(vert.pos);
        };
        auto print_triangle = [&](auto tri)
        {
            printf("[");
            print_vertex(vertices[tri.a]);
            printf(", ");
            print_vertex(vertices[tri.b]);
            printf(", ");
            print_vertex(vertices[tri.c]);
            printf("]");
        };
        
        for(auto & tri : triangles)
        {
            printf("face ");
            print_triangle(tri);
            printf("\n");
        }
    }
}
