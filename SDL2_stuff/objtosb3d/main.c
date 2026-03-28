#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN   1024
#define MAX_FACE_VERTS 64
#define MAX_MATERIALS  256

typedef struct {
    float x;
    float y;
    float z;
} Vec3File;

typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint8_t  color;         // gets the colour ID from the Material Name
    uint8_t  emission;      // { Ke - [Blender:Emission] }
    uint8_t  transparency;  // { d  - [Blender:Alpha] }
    uint8_t  shininess;     // 
} TriFile;

typedef struct {
    char     name[256];
    uint8_t  color;         // gets the colour ID from the Material Name
    uint8_t  emission;      // { Ke }
    uint8_t  transparency;  // { d  - [Blender:Alpha] }
    uint8_t  shininess;     // 
} MaterialInfo;

static int has_obj_extension(const char *filename)
{
    const char *dot;
    if (!filename) return 0;
    dot = strrchr(filename, '.');
    if (!dot) return 0;
    return (strcmp(dot, ".obj") == 0);
}

// Emission calculator //
static uint8_t ke_to_emission(float r, float g, float b)
{
    float v = r;
    const float scale = 255.0f;   /* adjust later if needed */

    if (g > v) v = g;
    if (b > v) v = b;

    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;

    v *= scale;

    if (v < 0.0f) v = 0.0f;
    if (v > 255.0f) v = 255.0f;

    return (uint8_t)(v + 0.5f);
}

// Transparency Calculator //
static uint8_t d_to_transparency(float d)
{
    if (d < 0.0f) d = 0.0f;
    if (d > 1.0f) d = 1.0f;

    return (uint8_t)((1.0f - d) * 255.0f + 0.5f);
}

static uint8_t ns_to_shininess(float ns)
{
    ns *= 0.001f;   // convert 0.0..1000.0 down to 0.0..1.0

    if (ns < 0.0f) ns = 0.0f;
    if (ns > 1.0f) ns = 1.0f;

    return (uint8_t)((1.0f - ns) * 255.0f + 0.5f);
}


static int build_output_name(const char *input, char *output, size_t outputSize)
{
    const char *dot;
    size_t baseLen;

    if (!input || !output || outputSize == 0) return 0;

    dot = strrchr(input, '.');
    if (!dot) return 0;

    baseLen = (size_t)(dot - input);

    if (baseLen + 6 > outputSize) return 0; /* ".sb3d" + nul */

    memcpy(output, input, baseLen);
    memcpy(output + baseLen, ".sb3d", 6);

    return 1;
}

static int build_path_from_obj(const char *objPath, const char *fileName, char *outPath, size_t outPathSize)
{
    const char *slash1;
    const char *slash2;
    const char *slash;
    size_t dirLen;
    size_t fileLen;

    if (!objPath || !fileName || !outPath || outPathSize == 0) return 0;

    slash1 = strrchr(objPath, '/');
    slash2 = strrchr(objPath, '\\');
    slash = slash1;

    if (slash2 && (!slash1 || slash2 > slash1)) {
        slash = slash2;
    }

    if (!slash) {
        fileLen = strlen(fileName);
        if (fileLen + 1 > outPathSize) return 0;
        memcpy(outPath, fileName, fileLen + 1);
        return 1;
    }

    dirLen = (size_t)(slash - objPath + 1);
    fileLen = strlen(fileName);

    if (dirLen + fileLen + 1 > outPathSize) return 0;

    memcpy(outPath, objPath, dirLen);
    memcpy(outPath + dirLen, fileName, fileLen + 1);

    return 1;
}


static int find_material(MaterialInfo *materials, int materialCount, const char *name)
{
    int i;

    for (i = 0; i < materialCount; i++) {
        if (strcmp(materials[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

static int parse_sbx_material_name(const char *name, uint8_t *outColor)
{
    const char *p;

    if (!name || !outColor) return 0;

    p = name;

    while (*p) {
        if (p[0] == 'S' &&
            p[1] == 'B' &&
            p[2] == 'X' &&
            p[3] == '_')
        {
            const char *n = p + 4;
            unsigned value = 0;
            int digits = 0;

            while (*n >= '0' && *n <= '9') {
                value = (value * 10u) + (unsigned)(*n - '0');
                if (value > 255u) return 0;
                digits++;
                n++;
            }

            if (digits == 0) {
                return 0;
            }

            *outColor = (uint8_t)value;
            return 1;
        }

        p++;
    }

    return 0;
}


static int load_mtl_file(const char *mtlPath, MaterialInfo *materials, int *materialCount)
{
    FILE *fp;
    char line[MAX_LINE_LEN];
    int current = -1;

    if (!mtlPath || !materials || !materialCount) return 0;

    fp = fopen(mtlPath, "r");
    if (!fp) {
        return 0;
    }

    *materialCount = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "newmtl ", 7) == 0) {
            char name[256];

            if (sscanf(line + 7, "%255s", name) == 1) {
                if (*materialCount >= MAX_MATERIALS) {
                    fclose(fp);
                    return 0;
                }

                memset(&materials[*materialCount], 0, sizeof(MaterialInfo));
                strncpy(materials[*materialCount].name, name, sizeof(materials[*materialCount].name) - 1);
                materials[*materialCount].name[sizeof(materials[*materialCount].name) - 1] = '\0';

                materials[*materialCount].color = 32;
                materials[*materialCount].emission = 0;
                materials[*materialCount].transparency = 0; // fully solid!!
                materials[*materialCount].shininess = 0;    // (NOT shiny)

                parse_sbx_material_name(
                    materials[*materialCount].name,
                    &materials[*materialCount].color
                );

                current = *materialCount;
                (*materialCount)++;
            }
        }
        else if (current >= 0) {
            float r, g, b, shine;
            float v;

            if (strncmp(line, "Ke ", 3) == 0) { // Emission {Blender uses Ke}
                if (sscanf(line + 3, "%f %f %f", &r, &g, &b) == 3) {
                    materials[current].emission = ke_to_emission(r, g, b);
                }
            }
            else if (strncmp(line, "d ", 2) == 0) {   /* Blender alpha {Blender uses d }*/
                if (sscanf(line + 2, "%f", &v) == 1) {
                    materials[current].transparency = d_to_transparency(v);
                }
            }
            else if (strncmp(line, "Ns ", 3) == 0) {    // blender Roughness!
                if (sscanf(line + 3, "%f", &shine) == 1) {
                    materials[current].shininess = ns_to_shininess(shine);
                }
            }
        }
    }

    fclose(fp);
    return 1;
}

static int load_materials_from_obj(const char *objPath, MaterialInfo *materials, int *materialCount)
{
    FILE *fp;
    char line[MAX_LINE_LEN];
    char mtlName[256];
    char mtlPath[1024];

    if (!objPath || !materials || !materialCount) return 0;

    *materialCount = 0;

    fp = fopen(objPath, "r");
    if (!fp) {
        return 0;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "mtllib ", 7) == 0) {
            if (sscanf(line + 7, "%255s", mtlName) == 1) {
                fclose(fp);

                if (!build_path_from_obj(objPath, mtlName, mtlPath, sizeof(mtlPath))) {
                    return 0;
                }

                return load_mtl_file(mtlPath, materials, materialCount);
            }
        }
    }

    fclose(fp);
    return 1; /* no mtllib is fine */
}

static int count_obj(FILE *fp, uint32_t *vertCount, uint32_t *triCount)
{
    char line[MAX_LINE_LEN];

    *vertCount = 0;
    *triCount  = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == 'v' && line[1] == ' ') {
            (*vertCount)++;
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            uint32_t faceVerts = 0;
            char *p = line + 2;

            while (*p) {
                while (*p == ' ' || *p == '\t') {
                    p++;
                }

                if (*p == '\0' || *p == '\r' || *p == '\n') {
                    break;
                }

                faceVerts++;

                while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
                    p++;
                }
            }

            if (faceVerts >= 3) {
                *triCount += (faceVerts - 2);
            }
        }
    }

    return (*vertCount > 0 && *triCount > 0);
}

static int parse_obj(
    FILE *fp,
    Vec3File *verts,
    uint32_t vertCapacity,
    TriFile *tris,
    uint32_t triCapacity,
    MaterialInfo *materials,
    int materialCount
)
{
    char line[MAX_LINE_LEN];
    uint32_t vi = 0;
    uint32_t ti = 0;
    uint8_t currentColor = 32;          // default colour
    uint8_t currentEmission = 0;        // no emission
    uint8_t currentTransparency = 0;    // solid
    uint8_t currentShine        = 0;    // MATT (not shiny bits)

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == 'v' && line[1] == ' ') {
            float x, y, z;

            if (sscanf(line + 2, "%f %f %f", &x, &y, &z) == 3) {
                if (vi >= vertCapacity) {
                    fprintf(stderr, "ERROR: vertex overflow while parsing\n");
                    return 0;
                }

                verts[vi].x = x;
                verts[vi].y = y;
                verts[vi].z = -z;   /* keep your current working axis fix */
                vi++;
            }
        }
        else if (strncmp(line, "usemtl ", 7) == 0) {
            char matName[256];
            int matIndex;

            matName[0] = '\0';

            if (sscanf(line + 7, "%255s", matName) == 1) {
                matIndex = find_material(materials, materialCount, matName);

                if (matIndex >= 0) {
                    currentColor = materials[matIndex].color;
                    currentEmission = materials[matIndex].emission;
                    currentTransparency = materials[matIndex].transparency;
                    currentShine = materials[matIndex].shininess;
                } else {
                    uint8_t parsedColor;

                    if (parse_sbx_material_name(matName, &parsedColor)) {
                        currentColor = parsedColor;
                    } else {
                        currentColor = 32;
                    }
                    currentEmission = 0;
                    currentTransparency = 0;
                    currentShine = 0;

                }
            }
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            int idx[MAX_FACE_VERTS];
            int n = 0;
            char *saveptr = NULL;
            char *tok = strtok_r(line + 2, " \t\r\n", &saveptr);

            while (tok && n < MAX_FACE_VERTS) {
                long vIndex = strtol(tok, NULL, 10); /* first integer before slash */
                idx[n++] = (int)vIndex;
                tok = strtok_r(NULL, " \t\r\n", &saveptr);
            }

            if (n >= 3) {
                int a0 = idx[0] - 1; /* OBJ is 1-based */

                for (int k = 1; k < n - 1; k++) {
                    int a1 = idx[k] - 1;
                    int a2 = idx[k + 1] - 1;

                    if (a0 < 0 || a1 < 0 || a2 < 0) {
                        fprintf(stderr, "ERROR: negative/invalid OBJ index not supported\n");
                        return 0;
                    }

                    if ((uint32_t)a0 >= vertCapacity ||
                        (uint32_t)a1 >= vertCapacity ||
                        (uint32_t)a2 >= vertCapacity) {
                        fprintf(stderr, "ERROR: face index out of range\n");
                        return 0;
                    }

                    if (ti >= triCapacity) {
                        fprintf(stderr, "ERROR: triangle overflow while parsing\n");
                        return 0;
                    }

                    tris[ti].a = (uint32_t)a0;
                    tris[ti].b = (uint32_t)a2;   /* keep your current working winding fix */
                    tris[ti].c = (uint32_t)a1;
                    tris[ti].color = currentColor;
                    tris[ti].emission = currentEmission;
                    tris[ti].transparency = currentTransparency;
                    tris[ti].shininess = currentShine;
                    ti++;
                }
            }
        }
    }

    if (vi != vertCapacity) {
        fprintf(stderr, "ERROR: parsed vertex count mismatch (%u vs %u)\n",
                (unsigned)vi, (unsigned)vertCapacity);
        return 0;
    }

    if (ti != triCapacity) {
        fprintf(stderr, "ERROR: parsed triangle count mismatch (%u vs %u)\n",
                (unsigned)ti, (unsigned)triCapacity);
        return 0;
    }

    return 1;
}

static int write_sb3d(
    const char *filename,
    const Vec3File *verts,
    uint32_t vertCount,
    const TriFile *tris,
    uint32_t triCount
)
{
    FILE *fp;
    const char magic[4] = { 'S', 'B', '3', 'D' };
    uint32_t version = 5;   // i know, total gash, but need to change this to prevent funny meshes ;)

    fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "ERROR: could not open output file '%s'\n", filename);
        return 0;
    }

    if (fwrite(magic, 1, 4, fp) != 4) goto write_fail;
    if (fwrite(&version, sizeof(version), 1, fp) != 1) goto write_fail;
    if (fwrite(&vertCount, sizeof(vertCount), 1, fp) != 1) goto write_fail;
    if (fwrite(&triCount, sizeof(triCount), 1, fp) != 1) goto write_fail;

    if (vertCount > 0) {
        if (fwrite(verts, sizeof(Vec3File), vertCount, fp) != vertCount) goto write_fail;
    }

    if (triCount > 0) {
        for (uint32_t i = 0; i < triCount; i++) {
            if (fwrite(&tris[i].a,           sizeof(uint32_t), 1, fp) != 1) goto write_fail;
            if (fwrite(&tris[i].b,           sizeof(uint32_t), 1, fp) != 1) goto write_fail;
            if (fwrite(&tris[i].c,           sizeof(uint32_t), 1, fp) != 1) goto write_fail;
            if (fwrite(&tris[i].color,        sizeof(uint8_t), 1, fp) != 1) goto write_fail;
            if (fwrite(&tris[i].emission,     sizeof(uint8_t), 1, fp) != 1) goto write_fail;
            if (fwrite(&tris[i].transparency, sizeof(uint8_t), 1, fp) != 1) goto write_fail;
            if (fwrite(&tris[i].shininess,    sizeof(uint8_t), 1, fp) != 1) goto write_fail;
        }
    }

    fclose(fp);
    return 1;

write_fail:
    fprintf(stderr, "ERROR: failed while writing '%s'\n", filename);
    fclose(fp);
    return 0;
}

int main(int argc, char *argv[])
{
    const char *inputName;
    char outputName[1024];
    FILE *fp;
    Vec3File *verts = NULL;
    TriFile *tris = NULL;
    MaterialInfo materials[MAX_MATERIALS];
    uint32_t vertCount = 0;
    uint32_t triCount = 0;
    int materialCount = 0;
    int ok = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file.obj\n", argv[0]);
        return 1;
    }

    inputName = argv[1];

    if (!has_obj_extension(inputName)) {
        fprintf(stderr, "ERROR: input must be a .obj file\n");
        return 1;
    }

    if (!build_output_name(inputName, outputName, sizeof(outputName))) {
        fprintf(stderr, "ERROR: could not build output filename\n");
        return 1;
    }

    if (!load_materials_from_obj(inputName, materials, &materialCount)) {
        fprintf(stderr, "WARNING: could not load MTL data, continuing without emission/material lookup\n");
        materialCount = 0;
    }

    fp = fopen(inputName, "r");
    if (!fp) {
        fprintf(stderr, "ERROR: could not open input file '%s'\n", inputName);
        return 1;
    }

    if (!count_obj(fp, &vertCount, &triCount)) {
        fprintf(stderr, "ERROR: failed to count vertices/triangles or file is empty\n");
        fclose(fp);
        return 1;
    }

    rewind(fp);

    verts = (Vec3File *)malloc(sizeof(Vec3File) * vertCount);
    tris  = (TriFile  *)malloc(sizeof(TriFile)  * triCount);

    if (!verts || !tris) {
        fprintf(stderr, "ERROR: out of memory\n");
        fclose(fp);
        free(verts);
        free(tris);
        return 1;
    }

    if (!parse_obj(fp, verts, vertCount, tris, triCount, materials, materialCount)) {
        fclose(fp);
        free(verts);
        free(tris);
        return 1;
    }

    fclose(fp);

    ok = write_sb3d(outputName, verts, vertCount, tris, triCount);

    if (!ok) {
        free(verts);
        free(tris);
        return 1;
    }

    printf("\n--- VERSION 5 ----\n");
    printf("Input : %s\n", inputName);
    printf("Output: %s\n", outputName);
    printf("Verts : %u\n", (unsigned)vertCount);
    printf("Tris  : %u\n", (unsigned)triCount);
    printf("Mats  : %d\n", materialCount);
    printf("END OF PROGRAM!\n");

    free(verts);
    free(tris);
    return 0;
}