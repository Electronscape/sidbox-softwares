#include <stdint.h>
#include <stdlib.h>
#include <math.h>


#include "sb3d.h"


Vec3 entityLocalToWorld(const Entity *e, Vec3 v)
{
    Vec3 world = e->pos;

    world = vec3Add(world, vec3Scale(e->right,   v.x));
    world = vec3Add(world, vec3Scale(e->up,      v.y));
    world = vec3Add(world, vec3Scale(e->forward, v.z));

    return world;
}


float meshComputeBoundsRadius(const Mesh *mesh)
{
    float maxDist2 = 0.0f;

    for (int i = 0; i < mesh->vertCount; i++) {
        Vec3 v = mesh->verts[i];
        float d2 = (v.x * v.x) + (v.y * v.y) + (v.z * v.z);

        if (d2 > maxDist2) {
            maxDist2 = d2;
        }
    }

    return sqrtf(maxDist2);
}


int entityCreate(Mesh *mesh, Vec3 pos)
{
    int id;

    if (worldEntityCount >= WORLD_MAX) {
        return -1;
    }

    id = worldEntityCount;
    worldEntityCount++;

    worldEntities[id].mesh = mesh;
    worldEntities[id].pos = pos;

    worldEntities[id].right   = (Vec3){ 1.0f, 0.0f, 0.0f };
    worldEntities[id].up      = (Vec3){ 0.0f, 1.0f, 0.0f };
    worldEntities[id].forward = (Vec3){ 0.0f, 0.0f, 1.0f };
    return id;
}


void entitySetPosition(int id, Vec3 pos)
{
    if (id < 0 || id >= worldEntityCount) return;
    worldEntities[id].pos = pos;
}

void entityMove(int id, Vec3 delta)
{
    if (id < 0 || id >= worldEntityCount) return;
    worldEntities[id].pos = vec3Add(worldEntities[id].pos, delta);
}

Vec3 getEntityPosition(int id){
    return worldEntities[id].pos;
}

void entityMoveForward(int id, float dist)
{
    if (id < 0 || id >= worldEntityCount) return;
    worldEntities[id].pos = vec3Add(
        worldEntities[id].pos,
        vec3Scale(worldEntities[id].forward, dist)
    );
}

void entityMoveRight(int id, float dist)
{
    if (id < 0 || id >= worldEntityCount) return;
    worldEntities[id].pos = vec3Add(
        worldEntities[id].pos,
        vec3Scale(worldEntities[id].right, dist)
    );
}

void entityMoveUp(int id, float dist)
{
    if (id < 0 || id >= worldEntityCount) return;
    worldEntities[id].pos = vec3Add(
        worldEntities[id].pos,
        vec3Scale(worldEntities[id].up, dist)
    );
}


// note colour palette SHOULD be 5 shades available, BUT not STRICTLY required ;)
// Set the whole mesh to a single colour
void entityColour(int id, uint8_t colour) {
    if (id < 0 || id >= worldEntityCount) return;

    Mesh *m = worldEntities[id].mesh;
    for (int i = 0; i < m->triCount; i++) {
        m->tris[i].color = colour;
    }
}

static void setMeshColour(Mesh *mesh, uint8_t colour){
    if(!mesh) return;

    for (int i = 0; i < mesh->triCount; i++) {
        mesh->tris[i].color = colour;
    }
}

// Set a specific face / triangle to a colour
void entityColourFace(int id, int faceId, uint8_t colour) {
    if (id < 0 || id >= worldEntityCount) return;

    Mesh *m = worldEntities[id].mesh;
    if (faceId < 0 || faceId >= m->triCount) return;

    m->tris[faceId].color = colour;
}


void normalizeEntity(Entity *e)
{
    e->forward = vec3Normalize(e->forward);
    e->right   = vec3Normalize(e->right);

    e->up = vec3Cross(e->forward, e->right);
    e->up = vec3Normalize(e->up);

    e->right = vec3Cross(e->up, e->forward);
    e->right = vec3Normalize(e->right);
}

void entityResetAxes(Entity *e)
{
    e->right   = (Vec3){ 1.0f, 0.0f, 0.0f };
    e->up      = (Vec3){ 0.0f, 1.0f, 0.0f };
    e->forward = (Vec3){ 0.0f, 0.0f, 1.0f };
}


void entityTurnLocal(int id, float yaw, float pitch, float roll)
{
    Entity *e;

    if (id < 0 || id >= worldEntityCount) return;
    e = &worldEntities[id];

    if (yaw != 0.0f) {
        e->forward = rotateAroundAxis(e->forward, e->up, yaw);
        e->right   = rotateAroundAxis(e->right,   e->up, yaw);
    }

    if (pitch != 0.0f) {
        e->forward = rotateAroundAxis(e->forward, e->right, pitch);
        e->up      = rotateAroundAxis(e->up,      e->right, pitch);
    }

    if (roll != 0.0f) {
        e->right = rotateAroundAxis(e->right,   e->forward, roll);
        e->up    = rotateAroundAxis(e->up,      e->forward, roll);
    }

    normalizeEntity(e);
}

void entityTurnGlobal(int id, float yaw, float pitch, float roll)
{
    Entity *e;
    Vec3 worldX = { 1.0f, 0.0f, 0.0f };
    Vec3 worldY = { 0.0f, 1.0f, 0.0f };
    Vec3 worldZ = { 0.0f, 0.0f, 1.0f };

    if (id < 0 || id >= worldEntityCount) return;
    e = &worldEntities[id];

    if (yaw != 0.0f) {
        e->forward = rotateAroundAxis(e->forward, worldY, yaw);
        e->right   = rotateAroundAxis(e->right,   worldY, yaw);
        e->up      = rotateAroundAxis(e->up,      worldY, yaw);
    }

    if (pitch != 0.0f) {
        e->forward = rotateAroundAxis(e->forward, worldX, pitch);
        e->right   = rotateAroundAxis(e->right,   worldX, pitch);
        e->up      = rotateAroundAxis(e->up,      worldX, pitch);
    }

    if (roll != 0.0f) {
        e->forward = rotateAroundAxis(e->forward, worldZ, roll);
        e->right   = rotateAroundAxis(e->right,   worldZ, roll);
        e->up      = rotateAroundAxis(e->up,      worldZ, roll);
    }

    normalizeEntity(e);
}

//////////// primative creator factory /////////////////////////////////
// Create a box mesh at origin with given width, height, depth
Mesh createBox(float width, float height, float depth)
{
    Mesh mesh;

    // Allocate vertices (8 corners)
    mesh.vertCount = 8;
    mesh.verts = malloc(sizeof(Vec3) * mesh.vertCount);

    float hw = width  * 0.5f;
    float hh = height * 0.5f;
    float hd = depth  * 0.5f;

    // Bottom vertices
    mesh.verts[0] = (Vec3){ -hw, -hh, -hd };
    mesh.verts[1] = (Vec3){  hw, -hh, -hd };
    mesh.verts[2] = (Vec3){  hw, -hh,  hd };
    mesh.verts[3] = (Vec3){ -hw, -hh,  hd };

    // Top vertices
    mesh.verts[4] = (Vec3){ -hw,  hh, -hd };
    mesh.verts[5] = (Vec3){  hw,  hh, -hd };
    mesh.verts[6] = (Vec3){  hw,  hh,  hd };
    mesh.verts[7] = (Vec3){ -hw,  hh,  hd };

    // Allocate edges (12 edges)
    mesh.edgeCount = 12;
    mesh.edges = malloc(sizeof(Edge) * mesh.edgeCount);
    Edge e[] = {
        {0,1},{1,2},{2,3},{3,0},     // bottom
        {4,5},{5,6},{6,7},{7,4},     // top
        {0,4},{1,5},{2,6},{3,7}      // verticals
    };
    for (int i = 0; i < mesh.edgeCount; i++) mesh.edges[i] = e[i];

    // Allocate triangles (12 tris, 2 per face)
    mesh.triCount = 12;
    mesh.tris = malloc(sizeof(Tri) * mesh.triCount);
    Tri t[] = {
        // bottom
        {0,1,2, DEFAULT_COLOUR_BOTTOM}, {0,2,3, DEFAULT_COLOUR_BOTTOM},
        // top
        {4,6,5, DEFAULT_COLOUR_TOP}, {4,7,6, DEFAULT_COLOUR_TOP},
        // side 1
        {0,4,5, DEFAULT_COLOUR_SIDE1}, {0,5,1, DEFAULT_COLOUR_SIDE1},
        // side 2
        {1,5,6, DEFAULT_COLOUR_SIDE2}, {1,6,2, DEFAULT_COLOUR_SIDE2},
        // side 3
        {2,6,7, DEFAULT_COLOUR_SIDE3}, {2,7,3, DEFAULT_COLOUR_SIDE3},
        // side 4
        {3,7,4, DEFAULT_COLOUR_SIDE4}, {3,4,0, DEFAULT_COLOUR_SIDE4}
    };
    for (int i = 0; i < mesh.triCount; i++) mesh.tris[i] = t[i];

    // Compute bounds radius
    mesh.boundsRadius = meshComputeBoundsRadius(&mesh);
    meshSetDefaultMaterial(&mesh);
    //setMeshColour(&mesh, 32);
    return mesh;
}




Mesh createSphere(float radius, int stacks, int slices) 
{
    Mesh mesh;
    

    mesh.vertCount = (stacks + 1) * (slices + 1);
    mesh.triCount = stacks * slices * 2;

    mesh.verts = malloc(sizeof(Vec3) * mesh.vertCount);
    mesh.tris  = malloc(sizeof(Tri) * mesh.triCount);
    // edges optional

    int vi = 0;
    for (int s = 0; s <= stacks; s++) {
        float phi = -M_PI/2 + (float)s / stacks * M_PI;
        float y = radius * sinf(phi);
        float r = radius * cosf(phi);
        for (int t = 0; t <= slices; t++) {
            float theta = (float)t / slices * 2.0f * M_PI;
            float x = r * sinf(theta);
            float z = r * cosf(theta);
            mesh.verts[vi++] = (Vec3){x, y, z};
        }
    }

    // generate triangles
    int ti = 0;
    for (int s = 0; s < stacks; s++) {
        for (int t = 0; t < slices; t++) {
            int i0 = s * (slices+1) + t;
            int i1 = i0 + slices + 1;
            int i2 = i0 + 1;
            int i3 = i1 + 1;

            mesh.tris[ti++] = (Tri){i0, i2, i1, DEFAULT_COLOUR};
            mesh.tris[ti++] = (Tri){i2, i3, i1, DEFAULT_COLOUR};
        }
    }

    mesh.boundsRadius = meshComputeBoundsRadius(&mesh);
    meshSetDefaultMaterial(&mesh);
    return mesh;
}


Mesh createPlane(float sizeX, float sizeZ, int divisions)
{
    Mesh mesh;   

    int vertsPerSide = divisions + 1;

    mesh.vertCount = vertsPerSide * vertsPerSide;
    mesh.triCount  = divisions * divisions * 2;

    mesh.verts = malloc(sizeof(Vec3) * mesh.vertCount);
    mesh.tris  = malloc(sizeof(Tri)  * mesh.triCount);
    mesh.edges = NULL;
    mesh.edgeCount = 0;

    float halfX = sizeX * 0.5f;
    float halfZ = sizeZ * 0.5f;

    float stepX = sizeX / divisions;
    float stepZ = sizeZ / divisions;

    // ---- generate vertices ----
    int vi = 0;

    for (int z = 0; z <= divisions; z++) {
        for (int x = 0; x <= divisions; x++) {

            float px = -halfX + (x * stepX);
            float pz = -halfZ + (z * stepZ);

            mesh.verts[vi++] = (Vec3){ px, 0.0f, pz };
        }
    }

    // ---- generate triangles ----
    int ti = 0;

    for (int z = 0; z < divisions; z++) {
        for (int x = 0; x < divisions; x++) {

            int i0 =  z      * vertsPerSide + x;
            int i1 = (z + 1) * vertsPerSide + x;
            int i2 =  i0 + 1;
            int i3 =  i1 + 1;

            mesh.tris[ti++] = (Tri){ i0, i1, i2, DEFAULT_COLOUR };
            mesh.tris[ti++] = (Tri){ i2, i1, i3, DEFAULT_COLOUR };
        }
    }
    mesh.boundsRadius = meshComputeBoundsRadius(&mesh);
    meshSetDefaultMaterial(&mesh);
    return mesh;
}


Mesh createCylinder(float radius, float height, int segments)
{
    Mesh mesh;
    int vertCount = (segments + 1) * 2; // top + bottom rings
    int triCount  = segments * 4;       // 2 for top cap, 2 for bottom cap, 2 per side quad
    mesh.vertCount = vertCount;
    mesh.triCount  = triCount;
    mesh.edgeCount = segments * 6;      // optional for wireframe edges

    mesh.verts = malloc(sizeof(Vec3) * vertCount);
    mesh.tris  = malloc(sizeof(Tri) * triCount);
    mesh.edges = malloc(sizeof(Edge) * mesh.edgeCount);

    float halfH = height * 0.5f;

    // vertices
    for (int i = 0; i <= segments; i++) {
        float theta = (float)i / segments * 2.0f * M_PI;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);

        // bottom ring
        mesh.verts[i] = (Vec3){ x, -halfH, z };
        // top ring
        mesh.verts[i + segments + 1] = (Vec3){ x, halfH, z };
    }

    int ti = 0;
    int ei = 0;

    // side quads -> 2 tris per segment
    for (int i = 0; i < segments; i++) {
        int b0 = i;
        int b1 = (i + 1) % (segments + 1);
        int t0 = b0 + segments + 1;
        int t1 = b1 + segments + 1;

        // first triangle
        mesh.tris[ti++] = (Tri){ b0, t0, t1, DEFAULT_COLOUR };
        // second triangle
        mesh.tris[ti++] = (Tri){ b0, t1, b1, DEFAULT_COLOUR };

        // edges for wireframe
        mesh.edges[ei++] = (Edge){ b0, b1 };
        mesh.edges[ei++] = (Edge){ t0, t1 };
        mesh.edges[ei++] = (Edge){ b0, t0 };
    }

    // top cap (normal points up)
    Vec3 topCenter = (Vec3){0, halfH, 0};
    mesh.verts = realloc(mesh.verts, sizeof(Vec3) * (mesh.vertCount + 1));
    mesh.verts[mesh.vertCount++] = topCenter;
    int topCenterIndex = mesh.vertCount - 1;

    for (int i = 0; i < segments; i++) {
        int t0 = i + segments + 1;
        int t1 = (i + 1) % (segments + 1) + segments + 1;
        // flip order for correct CCW
        mesh.tris[ti++] = (Tri){ topCenterIndex, t1, t0, DEFAULT_COLOUR };
    }

    // bottom cap (normal points down)
    Vec3 bottomCenter = (Vec3){0, -halfH, 0};
    mesh.verts = realloc(mesh.verts, sizeof(Vec3) * (mesh.vertCount + 1));
    mesh.verts[mesh.vertCount++] = bottomCenter;
    int bottomCenterIndex = mesh.vertCount - 1;

    for (int i = 0; i < segments; i++) {
        int b0 = i;
        int b1 = (i + 1) % (segments + 1);
        // flip order for correct CCW
        mesh.tris[ti++] = (Tri){ bottomCenterIndex, b0, b1, DEFAULT_COLOUR };
    }

    mesh.boundsRadius = meshComputeBoundsRadius(&mesh);
    meshSetDefaultMaterial(&mesh);

    return mesh;
}




Mesh createCone(float radius, float height, int segments)
{
    Mesh mesh;
    float halfH = height * 0.5f;

    // Vertices: bottom ring + tip
    mesh.vertCount = segments + 2;
    mesh.triCount  = segments * 2; // sides + bottom
    mesh.edgeCount = segments * 3;

    mesh.verts = malloc(sizeof(Vec3) * mesh.vertCount);
    mesh.tris  = malloc(sizeof(Tri) * mesh.triCount);
    mesh.edges = malloc(sizeof(Edge) * mesh.edgeCount);

    // bottom ring
    for (int i = 0; i <= segments; i++) {
        float theta = (float)i / segments * 2.0f * M_PI;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);
        mesh.verts[i] = (Vec3){ x, -halfH, z };
    }

    // tip vertex
    mesh.verts[mesh.vertCount - 1] = (Vec3){ 0, halfH, 0 };
    int tipIndex = mesh.vertCount - 1;

    int ti = 0;
    int ei = 0;

    // side triangles
    for (int i = 0; i < segments; i++) {
        int b0 = i;
        int b1 = (i + 1) % (segments + 1);
        // flip b0 and b1 to fix winding
        mesh.tris[ti++] = (Tri){ b1, b0, tipIndex, DEFAULT_COLOUR };

        mesh.edges[ei++] = (Edge){ b0, b1 };
        mesh.edges[ei++] = (Edge){ b0, tipIndex };
    }

    // bottom cap
    Vec3 bottomCenter = (Vec3){ 0, -halfH, 0 };
    mesh.verts = realloc(mesh.verts, sizeof(Vec3) * (mesh.vertCount + 1));
    mesh.verts[mesh.vertCount++] = bottomCenter;
    int bottomIndex = mesh.vertCount - 1;

    for (int i = 0; i < segments; i++) {
        int b0 = i;
        int b1 = (i + 1) % (segments + 1);
        mesh.tris[ti++] = (Tri){ bottomIndex, b0, b1, DEFAULT_COLOUR };
        mesh.edges[ei++] = (Edge){ b0, bottomIndex };
    }

    mesh.boundsRadius = meshComputeBoundsRadius(&mesh);
    meshSetDefaultMaterial(&mesh);
    return mesh;
}



Mesh createPyramid(float width, float height)
{
    Mesh mesh;
    mesh.vertCount = 5;
    mesh.triCount  = 6;
    mesh.edgeCount = 8;

    mesh.verts = malloc(sizeof(Vec3) * mesh.vertCount);
    mesh.tris  = malloc(sizeof(Tri) * mesh.triCount);
    mesh.edges = malloc(sizeof(Edge) * mesh.edgeCount);

    float hw = width * 0.5f;
    float hh = height * 0.5f;

    // base vertices
    mesh.verts[0] = (Vec3){ -hw, -hh, -hw };
    mesh.verts[1] = (Vec3){  hw, -hh, -hw };
    mesh.verts[2] = (Vec3){  hw, -hh,  hw };
    mesh.verts[3] = (Vec3){ -hw, -hh,  hw };

    // tip
    mesh.verts[4] = (Vec3){ 0, hh, 0 };

    // base
    mesh.tris[0] = (Tri){ 0, 1, 2, DEFAULT_COLOUR };
    mesh.tris[1] = (Tri){ 0, 2, 3, DEFAULT_COLOUR };

    // sides
    mesh.tris[2] = (Tri){ 0, 4, 1, DEFAULT_COLOUR };
    mesh.tris[3] = (Tri){ 1, 4, 2, DEFAULT_COLOUR };
    mesh.tris[4] = (Tri){ 2, 4, 3, DEFAULT_COLOUR };
    mesh.tris[5] = (Tri){ 3, 4, 0, DEFAULT_COLOUR };

    // edges (optional)
    mesh.edges[0] = (Edge){0,1}; mesh.edges[1] = (Edge){1,2};
    mesh.edges[2] = (Edge){2,3}; mesh.edges[3] = (Edge){3,0};
    mesh.edges[4] = (Edge){0,4}; mesh.edges[5] = (Edge){1,4};
    mesh.edges[6] = (Edge){2,4}; mesh.edges[7] = (Edge){3,4};

    mesh.boundsRadius = meshComputeBoundsRadius(&mesh);
    meshSetDefaultMaterial(&mesh);
    return mesh;
}





Mesh createTorus(float majorRadius, float minorRadius, int majorSegs, int minorSegs)
{
    Mesh mesh;
    mesh.vertCount = (majorSegs + 1) * (minorSegs + 1);
    mesh.triCount  = majorSegs * minorSegs * 2;
    mesh.edgeCount = 0; // optional

    mesh.verts = malloc(sizeof(Vec3) * mesh.vertCount);
    mesh.tris  = malloc(sizeof(Tri) * mesh.triCount);
    mesh.edges = NULL;

    int vi = 0;
    for (int i = 0; i <= majorSegs; i++) {
        float phi = (float)i / majorSegs * 2.0f * M_PI;
        float cosPhi = cosf(phi);
        float sinPhi = sinf(phi);

        for (int j = 0; j <= minorSegs; j++) {
            float theta = (float)j / minorSegs * 2.0f * M_PI;
            float cosTheta = cosf(theta);
            float sinTheta = sinf(theta);

            float x = (majorRadius + minorRadius * cosTheta) * cosPhi;
            float y = minorRadius * sinTheta;
            float z = (majorRadius + minorRadius * cosTheta) * sinPhi;

            mesh.verts[vi++] = (Vec3){x, y, z};
        }
    }

    int ti = 0;
    for (int i = 0; i < majorSegs; i++) {
        for (int j = 0; j < minorSegs; j++) {
            int i0 = i * (minorSegs + 1) + j;
            int i1 = i0 + minorSegs + 1;
            int i2 = i0 + 1;
            int i3 = i1 + 1;

            mesh.tris[ti++] = (Tri){i0, i2, i1, DEFAULT_COLOUR};
            mesh.tris[ti++] = (Tri){i2, i3, i1, DEFAULT_COLOUR};
        }
    }

    mesh.boundsRadius = meshComputeBoundsRadius(&mesh);
    meshSetDefaultMaterial(&mesh);
    return mesh;
}




void meshSetVertex(Mesh *mesh, int index, Vec3 v)
{
    if (!mesh) return;
    if (index < 0 || index >= mesh->vertCount) return;

    mesh->verts[index] = v;
}

void meshOffsetVertex(Mesh *mesh, int index, Vec3 delta)
{
    if (!mesh) return;
    if (index < 0 || index >= mesh->vertCount) return;

    mesh->verts[index] = vec3Add(mesh->verts[index], delta);
}

Vec3 meshGetVertex(const Mesh *mesh, int index)
{
    if (!mesh) return (Vec3){0.0f, 0.0f, 0.0f};
    if (index < 0 || index >= mesh->vertCount) return (Vec3){0.0f, 0.0f, 0.0f};

    return mesh->verts[index];
}

void meshSetVertexRecalc(Mesh *mesh, int index, Vec3 v)
{
    if (!mesh) return;
    if (index < 0 || index >= mesh->vertCount) return;

    mesh->verts[index] = v;
    mesh->boundsRadius = meshComputeBoundsRadius(mesh);
}

void meshOffsetVertexRecalc(Mesh *mesh, int index, Vec3 delta)
{
    if (!mesh) return;
    if (index < 0 || index >= mesh->vertCount) return;

    mesh->verts[index] = vec3Add(mesh->verts[index], delta);
    mesh->boundsRadius = meshComputeBoundsRadius(mesh);
}

void meshResetFromSource(Mesh *dst, const Mesh *src)
{
    if (!dst || !src) return;
    if (dst->vertCount != src->vertCount) return;

    for (int i = 0; i < src->vertCount; i++) {
        dst->verts[i] = src->verts[i];
    }

    dst->boundsRadius = src->boundsRadius;
}






// demo test
void meshDeformWaveY(Mesh *mesh, float time, float amount, float freq)
{
    if (!mesh) return;

    for (int i = 0; i < mesh->vertCount; i++) {
        Vec3 v = mesh->verts[i];
        v.y += sinf(time + (v.x * freq)) * amount;
        mesh->verts[i] = v;
    }

    mesh->boundsRadius = meshComputeBoundsRadius(mesh);
}

void meshDeformWavePlaneY(Mesh *mesh, float time, float amp, float freqX, float freqZ, float speed)
{
    if (!mesh) return;

    for (int i = 0; i < mesh->vertCount; i++) {
        Vec3 v = mesh->verts[i];

        float wave =
            sinf((v.x * freqX) + (time * speed)) +
            cosf((v.z * freqZ) + (time * speed * 0.7f));

        v.y += wave * amp;
        mesh->verts[i] = v;
    }

    mesh->boundsRadius = meshComputeBoundsRadius(mesh);
}





// Deep copy a mesh (returns independent instance)
Mesh copyMesh(const Mesh *src)
{
    Mesh dst;

    dst.vertCount = src->vertCount;
    dst.edgeCount = src->edgeCount;
    dst.triCount  = src->triCount;

    dst.verts = NULL;
    dst.edges = NULL;
    dst.tris  = NULL;

    if (dst.vertCount > 0) {
        dst.verts = malloc(sizeof(Vec3) * dst.vertCount);
    }

    if (dst.edgeCount > 0) {
        dst.edges = malloc(sizeof(Edge) * dst.edgeCount);
    }

    if (dst.triCount > 0) {
        dst.tris = malloc(sizeof(Tri) * dst.triCount);
    }

    for (int i = 0; i < dst.vertCount; i++) {
        dst.verts[i] = src->verts[i];
    }

    for (int i = 0; i < dst.edgeCount; i++) {
        dst.edges[i] = src->edges[i];
    }

    for (int i = 0; i < dst.triCount; i++) {
        dst.tris[i] = src->tris[i];
    }

    dst.boundsRadius = src->boundsRadius;
    dst.material     = src->material;

    return dst;
}


void meshSetDefaultMaterial(Mesh *mesh)
{
    if (!mesh) return;

    mesh->material.ambient          = 0.00f;
    mesh->material.diffuse          = 1.00f;
    mesh->material.emissive         = 0.00f;
    mesh->material.specularStrength = 0.00f;
    mesh->material.shininess        = 8.0f;
}

void meshSetMaterial(
    Mesh *mesh,
    float ambient,
    float diffuse,
    float emissive,
    float specularStrength,
    float shininess
)
{
    if (!mesh) return;

    mesh->material.ambient          = ambient;
    mesh->material.diffuse          = diffuse;
    mesh->material.emissive         = emissive;
    mesh->material.specularStrength = specularStrength;
    mesh->material.shininess        = shininess;
}


void entityFollowCameraXZ(int id, const Camera *cam, float worldY, float snap)
{
    float px;
    float pz;

    if (!cam) return;
    if (id < 0 || id >= worldEntityCount) return;

    if (snap <= 0.0f) {
        snap = 1.0f;
    }

    px = floorf(cam->pos.x / snap) * snap;
    pz = floorf(cam->pos.z / snap) * snap;

    worldEntities[id].pos.x = px;
    worldEntities[id].pos.y = worldY;
    worldEntities[id].pos.z = pz;
}

