#include "../GameCommon/framework.h"

#include "../GameCommon/CBmpImage.h"
#include "../GameCommon/CBrush.h"
#include "../GameCommon/CCollision.h"
#include "../GameCommon/CDirectoryList.h"
#include "../GameCommon/CEntity.h"
#include "../GameCommon/CHeap.h"
#include "../GameCommon/CHeapArray.h"
#include "../GameCommon/CLinkList.h"
#include "../GameCommon/CLocal.h"
#include "../GameCommon/CSector.h"
#include "../GameCommon/CShaderMaterial.h"
#include "../GameCommon/CTerrainTile.h"
#include "../GameCommon/CTgaImage.h"
#include "../GameCommon/CToken.h"
#include "../GameCommon/CVec3f.h"
#include "../GameCommon/CVertexNT.h"

CLocal m_local;

errno_t m_err;

CDirectoryList* m_directoryList;

CCollision m_collision;
CSector* m_sector;
CVec3f m_sectorTriangle[12][3];
CVec3f m_sectorNormal[12];
CVec3f m_dir;

CVec3i m_mapSize = {};
int m_sectorSize = 0;

CEntity m_entity[MAX_ENTITIES];

CShaderMaterial m_masterMaterials[MAX_MATERIALS];
CShaderMaterial m_materials[MAX_MATERIALS];
CTgaImage m_images[MAX_MATERIALS];

CToken m_mapScript;
CToken m_shaderScript;
CToken m_objectScript;

FILE* m_fMap;
FILE* m_fCollision;

char m_directory[LONG_STRING] = {};
char m_mapName[LONG_STRING] = {};
char m_mapShortName[SHORT_STRING] = {};

char m_shaderDirectory[LONG_STRING] = {};
char m_textureDirectory[LONG_STRING] = {};
char m_mapDirectory[LONG_STRING] = {};
char m_collisionDirectory[LONG_STRING] = {};

char m_key[SHORT_STRING] = {};
char m_value[LONG_STRING] = {};

int m_brushCount = 0;
int m_entityCount = 0;
int m_entityMapCount = 0;
int m_entityCollisionCount = 0;
int m_materialsCount = 0;
int m_masterMaterialsCount = 0;

CHeap* m_heapMap[MAX_MATERIALS];
CHeap* m_heapCollision[MAX_MATERIALS];

bool m_wroteHeader[MAX_MATERIALS];






CString* filename = nullptr;

CHeapArray* verts = nullptr;
CHeapArray* tiles = nullptr;
CBmpImage* bmp = nullptr;
CBmpImage* mask = nullptr;
CBmpImage* lightmap = nullptr;

float vheight = {};

int primSize = {};

int useUV = {};
int average = {};

BYTE* pixelStart;

int vertexCount;
DWORD b;

float x;
float z;

float uinc;
float vinc;

float u;
float v;

int width;
int height;

CTerrainTile t1;
CTerrainTile t2;
CTerrainTile t3;
CTerrainTile t4;

FILE* master;
FILE* verticesFile;
FILE* maskFile;
FILE* lightmapFile;

CHeap* m_terrainCollisionHeap = {};

CVec3f down = CVec3f(0.0f, -1.0f, 0.0f);

CVec3f m_origin;







void LoadAllShaders();
void ParseShaderScript();
void WriteMaterials();

bool CheckTriangleOutsideOfSector(CVec3f* point, CVec3f* sector);
bool CheckPointInSector(int x, int y, int z, CVec3f* pp, CVec3f* n, CVec3f* sectorPoint);
bool CheckTriangleRayTrace(int x, int y, int z, CVec3f* sectorPoint, CVec3f* pp, CVec3f* n);
bool CheckSectorRayTrace(int x, int y, int z, CVec3f* sectorPoint, CVec3f* pp, CVec3f* n);
void WriteCollisionPrimitive(const char* t, int x, int y, int z, CVec3f* pp, CVec3f* n);
void WriteBrush(CBrush* brush);
void WriteCollisionPrimitiveToSectors(CVec3f* heap);

void CreateTerrain(CEntity* entity);
void WriteTile(CVec3f* A, CVec3f* B, CVec3f* C, CVec3f* n1, CVec3f* D, CVec3f* E, CVec3f* F, CVec3f* n2, bool uuv, float tu, float tv);
void WriteNormal(CVec3f* n);
void WriteUVCoord(float tu, float tv);
/*
*/
int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		printf("Input parameters should be directory then map name\n");

		return 0;
	}

	memcpy(m_directory, argv[1], strlen(argv[1]));
	memcpy(m_mapName, argv[2], strlen(argv[2]));

	memcpy(m_mapDirectory, m_directory, strlen(m_directory));
	memcpy(m_collisionDirectory, m_directory, strlen(m_directory));

	strncat_s(m_mapDirectory, LONG_STRING, "main/maps/", 10);
	strncat_s(m_collisionDirectory, LONG_STRING, "main/maps/", 10);

	int slash = 0;

	for (int i = 0;i < strlen(m_mapName);i++)
	{
		if (m_mapName[i] == '/')
		{
			slash = i;
		}
	}

	memset(m_mapShortName, 0x00, SHORT_STRING);
	memcpy(m_mapShortName, &m_mapName[slash + 1], strlen(&m_mapName[slash + 1]) - 4);

	strncat_s(m_mapDirectory, LONG_STRING, m_mapShortName, strlen(m_mapShortName));
	strncat_s(m_mapDirectory, LONG_STRING, ".dat", 4);

	strncat_s(m_collisionDirectory, LONG_STRING, m_mapShortName, strlen(m_mapShortName));
	strncat_s(m_collisionDirectory, LONG_STRING, ".col", 4);

	m_err = fopen_s(&m_fMap, m_mapDirectory, "wb");

	if (m_err != 0)
	{
		printf("Error %i opening:%s\n", m_err, m_mapDirectory);
		return 0;
	}

	m_err = fopen_s(&m_fCollision, m_collisionDirectory, "wb");

	if (m_err != 0)
	{
		printf("Error %i opening:%s\n", m_err, m_collisionDirectory);
		return 0;
	}

	LoadAllShaders();

	m_mapScript.InitBuffer(m_mapName);

	while (!m_mapScript.CheckEndOfBuffer())
	{
		// is it a comment
		if (strncmp(m_mapScript.m_buffer, "//", 2) == 0)
		{
			m_mapScript.SkipEndOfLine();
		}

		// left brace starts an entity
		if (strncmp(m_mapScript.m_buffer, "{", 1) == 0)
		{
			m_mapScript.SkipEndOfLine();

			m_entity[m_entityCount].Initialize(NULL);

			// while not at the end of the entity
			while (strncmp(m_mapScript.m_buffer, "}", 1) != 0)
			{
				// is it a key value pair
				if (strncmp(m_mapScript.m_buffer, "\"", 1) == 0)
				{
					memset(m_key, 0x00, SHORT_STRING);

					char* key = m_mapScript.GetQuotedToken();
					key++;
					memcpy(m_key, key, strlen(key) - 1);

					m_mapScript.Move(1);

					memset(m_value, 0x00, LONG_STRING);

					char* value = m_mapScript.GetQuotedToken();
					value++;
					memcpy(m_value, value, strlen(value) - 1);

					m_entity[m_entityCount].AddKeyValue(m_key, m_value);

					m_mapScript.SkipEndOfLine();
				}

				// is it a comment
				if (strncmp(m_mapScript.m_buffer, "//", 2) == 0)
				{
					m_mapScript.SkipEndOfLine();
				}

				// is it a brush
				if (strncmp(m_mapScript.m_buffer, "{", 1) == 0)
				{
					m_mapScript.SkipEndOfLine();

					if (strncmp(m_mapScript.m_buffer, "patchDef2", 9) == 0)
					{
						while (strncmp(m_mapScript.m_buffer, "}", 1) != 0)
						{
							m_mapScript.SkipEndOfLine();
						}
					}
					else
					{
						CBrush* brush = new CBrush();

						brush->m_fMap = m_fMap;
						brush->m_fCollision = m_fCollision;
						brush->m_local = &m_local;
						brush->m_masterMaterials = m_masterMaterials;
						brush->m_masterMaterialsCount = &m_masterMaterialsCount;
						brush->m_materials = m_materials;
						brush->m_materialsCount = &m_materialsCount;
						brush->m_images = m_images;
						brush->m_mapScript = &m_mapScript;

						brush->ParseScript();
						brush->BuildSides();

#ifdef _DEBUG
						printf("brush: %i\n\n", m_brushCount);

						brush->PrintInfo();
#endif
						brush->m_number = m_brushCount;

						m_entity[m_entityCount].m_brushes->Append(brush, m_brushCount);

						m_brushCount++;

						m_mapScript.SkipEndOfLine();
					}
				}
			}

			m_entity[m_entityCount].m_number = m_entityCount;

			switch (m_entity[m_entityCount].m_type)
			{
			case CEntity::Type::WORLDSPAWN:
			case CEntity::Type::COLLECTABLE:
			case CEntity::Type::TERRAIN:
			{
				m_entityMapCount++;
				m_entityCollisionCount++;

				break;
			}
			case CEntity::Type::LIGHT:
			case CEntity::Type::STATICMODEL:
			{
				m_entityMapCount++;

				break;
			}
			case CEntity::Type::INFOPLAYERSTART:
			{
				m_entityCollisionCount++;

				break;
			}
			}

			m_entityCount++;
		}

		m_mapScript.SkipEndOfLine();
	}

	printf("brush count:%i\n\n", m_brushCount);


	m_entity[CEntity::Type::WORLDSPAWN].GetKeyValue("mapSize", &m_mapSize);
	m_entity[CEntity::Type::WORLDSPAWN].GetKeyValue("sectorSize", &m_sectorSize);

	m_sector = new CSector(m_mapSize.m_p.x, m_mapSize.m_p.z, m_mapSize.m_p.y, m_sectorSize);


	WriteMaterials();

	fwrite(&m_entityMapCount, sizeof(int), 1, m_fMap);
	fwrite(&m_entityCollisionCount, sizeof(int), 1, m_fCollision);

	for (int i = 0; i < m_entityCount; i++)
	{
		m_entity[i].m_sector = m_sector;

		switch (m_entity[i].m_type)
		{
		case CEntity::Type::WORLDSPAWN:
		{
			m_entity[i].WriteKeyValues(m_fMap);
			m_entity[i].WriteKeyValues(m_fCollision);

			CLinkListNode<CBrush>* brush = m_entity[i].m_brushes->m_list;

			while (brush->m_object)
			{
				WriteBrush(brush->m_object);

				brush = brush->m_next;
			}

			fwrite(&MINUS_ONE, sizeof(int), 1, m_fMap);
			fwrite(&MINUS_ONE, sizeof(int), 1, m_fCollision);

			break;
		}
		case CEntity::Type::INFOPLAYERSTART:
		{
			m_entity[i].WriteKeyValues(m_fCollision);

			break;
		}
		case CEntity::Type::COLLECTABLE:
		{
			m_entity[i].WriteKeyValues(m_fMap);
			m_entity[i].WriteKeyValues(m_fCollision);

			break;
		}
		case CEntity::Type::LIGHT:
		case CEntity::Type::STATICMODEL:
		{
			m_entity[i].WriteKeyValues(m_fMap);

			break;
		}
		case CEntity::Type::TERRAIN:
		{
			m_entity[i].WriteKeyValues(m_fMap);
			m_entity[i].WriteKeyValues(m_fCollision);

			CreateTerrain(&m_entity[i]);

			fwrite(&MINUS_ONE, sizeof(int), 1, m_fCollision);

			break;
		}
		default:
		{
			for (int e = 0; e < m_entity[i].m_keyValueCount; e++)
			{
				printf("Unkown entity:%s %s\n", m_entity[i].m_keyValue[e].m_key, m_entity[i].m_keyValue[e].m_value);
			}

			break;
		}
		}
	}

	fclose(m_fMap);
	fclose(m_fCollision);

	delete m_sector;

	return 0;
}

/*
*/
void LoadAllShaders()
{
	m_masterMaterialsCount = 0;

	memcpy(m_shaderDirectory, m_directory, strlen(m_directory));
	strncat_s(m_shaderDirectory, LONG_STRING, "main/scripts/", 13);

	m_directoryList = new CDirectoryList();

	m_directoryList->LoadFromDirectory(m_shaderDirectory, "shader");

	CLinkListNode<CString>* shader = m_directoryList->m_directories->m_list;

	while (shader->m_object)
	{
		m_shaderScript.InitBuffer(shader->m_object->m_text);

		ParseShaderScript();

		shader = shader->m_next;
	}

	delete m_directoryList;
}

/*
*/
void ParseShaderScript()
{
	while (!m_shaderScript.CheckEndOfBuffer())
	{
		if (strncmp(m_shaderScript.m_buffer, "textures/", 9) == 0)
		{
			char* n = m_shaderScript.GetToken();

			if (m_masterMaterialsCount == MAX_MATERIALS)
			{
				printf("Exceeded maximum materials:%i\n", m_masterMaterialsCount);

				return;
			}

			m_masterMaterials[m_masterMaterialsCount].SetName(n);

			m_shaderScript.SkipEndOfLine();

			bool hasDiffuse = false;

			while (strncmp(m_shaderScript.m_buffer, "}", 1) != 0)
			{
				if (strncmp(m_shaderScript.m_buffer, "surface", 7) == 0)
				{
					m_shaderScript.Move(7);

					m_shaderScript.SkipEndOfLine();
				}
				else if (strncmp(m_shaderScript.m_buffer, "implicitMap", 11) == 0)
				{
					m_shaderScript.Move(11);

					n = m_shaderScript.GetToken();

					memset(m_textureDirectory, 0x00, LONG_STRING);

					strncat_s(m_textureDirectory, LONG_STRING, n, strlen(n));

					m_masterMaterials[m_masterMaterialsCount].SetMapKd(m_textureDirectory);

					m_shaderScript.SkipEndOfLine();
				}
				else if (strncmp(m_shaderScript.m_buffer, "diffuse", 7) == 0)
				{
					m_shaderScript.Move(7);

					hasDiffuse = true;

					float r = (float)atof(m_shaderScript.GetToken());
					float g = (float)atof(m_shaderScript.GetToken());
					float b = (float)atof(m_shaderScript.GetToken());
					float a = (float)atof(m_shaderScript.GetToken());

					m_masterMaterials[m_masterMaterialsCount].SetKd(r, g, b, a);

					m_shaderScript.SkipEndOfLine();
				}
				else
				{
					m_shaderScript.Move(1);
				}
			}

			if (!hasDiffuse)
			{
				m_masterMaterials[m_masterMaterialsCount].SetKd(1.0f, 1.0f, 1.0f, 1.0f);
			}

			m_masterMaterialsCount++;
		}

		m_shaderScript.SkipEndOfLine();
	}
}

/*
*/
void WriteMaterials()
{
	fwrite(&m_materialsCount, sizeof(int), 1, m_fMap);

#ifdef _DEBUG
	printf("materials:%i\n\n", m_materialsCount);
#endif

	for (int m = 0; m < m_materialsCount; m++)
	{
#ifdef _DEBUG
		printf("%03i %s\n", m_materials[m].m_number, m_materials[m].m_name);
#endif
		fwrite(&m_materials[m], sizeof(CShaderMaterial), 1, m_fMap);
	}
}

/*
*/
void WriteBrush(CBrush* brush)
{
	for (int i = 0; i < brush->m_sideCount; i++)
	{
		brush->m_totalVertices[brush->m_brushSide[i].m_shaderMaterial->m_number] += brush->m_brushSide[i].CountVertices();
	}

	for (int i = 0; i < m_materialsCount; i++)
	{
		m_wroteHeader[i] = false;

		if (brush->m_totalVertices[i] != 0)
		{
			m_heapMap[i] = new CHeap(nullptr, 32768);
			m_heapCollision[i] = new CHeap(nullptr, 32768);
		}
	}

	for (int i = 0; i < m_materialsCount; i++)
	{
		if (brush->m_totalVertices[i] != 0)
		{
			for (int s = 0; s < brush->m_sideCount; s++)
			{
				if (brush->m_brushSide[s].m_shaderMaterial->m_number == i)
				{
					if (!m_wroteHeader[i])
					{
						m_heapMap[i]->Append(brush->m_brushSide[s].m_shaderMaterial->m_number);

						m_heapMap[i]->Append(brush->m_totalVertices[i]);

						m_wroteHeader[i] = true;
					}

					for (int v = 0; v < brush->m_brushSide[s].m_vertexCount; v++)
					{
						m_heapMap[i]->Append((unsigned char*)&brush->m_brushSide[s].m_center.m_v, sizeof(CVertexNT));


						m_heapCollision[i]->Append((unsigned char*)&brush->m_brushSide[s].m_center.m_v.m_p, sizeof(XMFLOAT3));


						if (v == (brush->m_brushSide[s].m_vertexCount - 1))
						{
							m_heapMap[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v].m_v, sizeof(CVertexNT));
							m_heapMap[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[0].m_v, sizeof(CVertexNT));

							m_heapCollision[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v].m_v.m_p, sizeof(XMFLOAT3));
							m_heapCollision[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[0].m_v.m_p, sizeof(XMFLOAT3));
						}
						else
						{
							m_heapMap[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v].m_v, sizeof(CVertexNT));
							m_heapMap[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v + 1].m_v, sizeof(CVertexNT));

							m_heapCollision[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v].m_v.m_p, sizeof(XMFLOAT3));
							m_heapCollision[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v + 1].m_v.m_p, sizeof(XMFLOAT3));
						}

						m_heapCollision[i]->Append((unsigned char*)&brush->m_brushSide[s].m_center.m_v.m_n, sizeof(XMFLOAT3));
					}
				}
			}
		}
	}


	for (int i = 0; i < m_materialsCount; i++)
	{
		if (m_heapMap[i] != nullptr)
		{
			fwrite(m_heapMap[i]->m_heap, m_heapMap[i]->m_length, 1, m_fMap);

			delete m_heapMap[i];

			m_heapMap[i] = nullptr;
		}

		if (m_heapCollision[i] != nullptr)
		{
			unsigned char* heap = m_heapCollision[i]->m_heap;

			while (heap < (m_heapCollision[i]->m_heap + m_heapCollision[i]->m_length))
			{
				WriteCollisionPrimitiveToSectors((CVec3f*)heap);

				heap += sizeof(CVec3f) * 4;
			}

			delete m_heapCollision[i];

			m_heapCollision[i] = nullptr;
		}
	}
}

/*
*/
void WriteCollisionPrimitiveToSectors(CVec3f* heap)
{
	CVec3f* points = heap;
	CVec3f* normal = &heap[3];

	float sx = (float)(m_sector->m_width / 2) - m_sector->m_width;
	float sy = (float)(m_sector->m_vertical / 2) - m_sector->m_vertical;
	float sz = (float)(m_sector->m_height / 2) - m_sector->m_height;

	CVec3f sectorVolume[8] = {};

	sectorVolume[0] = CVec3f(sx, sy, sz);
	sectorVolume[1] = CVec3f(sx, sy, sz + m_sector->m_sectorSize);
	sectorVolume[2] = CVec3f(sx + m_sector->m_sectorSize, sy, sz + m_sector->m_sectorSize);
	sectorVolume[3] = CVec3f(sx + m_sector->m_sectorSize, sy, sz);

	sectorVolume[4] = CVec3f(sx, sy + m_sector->m_sectorSize, sz);
	sectorVolume[5] = CVec3f(sx, sy + m_sector->m_sectorSize, sz + m_sector->m_sectorSize);
	sectorVolume[6] = CVec3f(sx + m_sector->m_sectorSize, sy + m_sector->m_sectorSize, sz + m_sector->m_sectorSize);
	sectorVolume[7] = CVec3f(sx + m_sector->m_sectorSize, sy + m_sector->m_sectorSize, sz);

	for (UINT y = 0; y < m_sector->m_gridVertical; y++)
	{
		for (UINT z = 0; z < m_sector->m_gridHeight; z++)
		{
			for (UINT x = 0; x < m_sector->m_gridWidth; x++)
			{
				if (CheckTriangleOutsideOfSector(points, sectorVolume))
				{
				}
				else if (CheckPointInSector(x, y, z, points, normal, sectorVolume))
				{
				}
				else if (CheckSectorRayTrace(x, y, z, sectorVolume, points, normal))
				{
				}
				else
				{
					CheckTriangleRayTrace(x, y, z, sectorVolume, points, normal);
				}

				sectorVolume[0].m_p.x += m_sector->m_sectorSize;
				sectorVolume[1].m_p.x += m_sector->m_sectorSize;
				sectorVolume[2].m_p.x += m_sector->m_sectorSize;
				sectorVolume[3].m_p.x += m_sector->m_sectorSize;

				sectorVolume[4].m_p.x += m_sector->m_sectorSize;
				sectorVolume[5].m_p.x += m_sector->m_sectorSize;
				sectorVolume[6].m_p.x += m_sector->m_sectorSize;
				sectorVolume[7].m_p.x += m_sector->m_sectorSize;
			}

			sectorVolume[0].m_p.x = sx;
			sectorVolume[1].m_p.x = sx;
			sectorVolume[2].m_p.x = sx + m_sector->m_sectorSize;
			sectorVolume[3].m_p.x = sx + m_sector->m_sectorSize;

			sectorVolume[4].m_p.x = sx;
			sectorVolume[5].m_p.x = sx;
			sectorVolume[6].m_p.x = sx + m_sector->m_sectorSize;
			sectorVolume[7].m_p.x = sx + m_sector->m_sectorSize;

			sectorVolume[0].m_p.z += m_sector->m_sectorSize;
			sectorVolume[1].m_p.z += m_sector->m_sectorSize;
			sectorVolume[2].m_p.z += m_sector->m_sectorSize;
			sectorVolume[3].m_p.z += m_sector->m_sectorSize;

			sectorVolume[4].m_p.z += m_sector->m_sectorSize;
			sectorVolume[5].m_p.z += m_sector->m_sectorSize;
			sectorVolume[6].m_p.z += m_sector->m_sectorSize;
			sectorVolume[7].m_p.z += m_sector->m_sectorSize;
		}

		sectorVolume[0].m_p.x = sx;
		sectorVolume[1].m_p.x = sx;
		sectorVolume[2].m_p.x = sx + m_sector->m_sectorSize;
		sectorVolume[3].m_p.x = sx + m_sector->m_sectorSize;

		sectorVolume[4].m_p.x = sx;
		sectorVolume[5].m_p.x = sx;
		sectorVolume[6].m_p.x = sx + m_sector->m_sectorSize;
		sectorVolume[7].m_p.x = sx + m_sector->m_sectorSize;

		sectorVolume[0].m_p.z = sz;
		sectorVolume[1].m_p.z = sz + m_sector->m_sectorSize;
		sectorVolume[2].m_p.z = sz + m_sector->m_sectorSize;
		sectorVolume[3].m_p.z = sz;

		sectorVolume[4].m_p.z = sz;
		sectorVolume[5].m_p.z = sz + m_sector->m_sectorSize;
		sectorVolume[6].m_p.z = sz + m_sector->m_sectorSize;
		sectorVolume[7].m_p.z = sz;

		sectorVolume[0].m_p.y += m_sector->m_sectorSize;
		sectorVolume[1].m_p.y += m_sector->m_sectorSize;
		sectorVolume[2].m_p.y += m_sector->m_sectorSize;
		sectorVolume[3].m_p.y += m_sector->m_sectorSize;

		sectorVolume[4].m_p.y += m_sector->m_sectorSize;
		sectorVolume[5].m_p.y += m_sector->m_sectorSize;
		sectorVolume[6].m_p.y += m_sector->m_sectorSize;
		sectorVolume[7].m_p.y += m_sector->m_sectorSize;
	}
}

/*
*/
bool CheckTriangleOutsideOfSector(CVec3f* point, CVec3f* sector)
{
	if (point[0].m_p.x < sector[0].m_p.x)
		if (point[0].m_p.y < sector[0].m_p.y)
			if (point[0].m_p.z < sector[0].m_p.z)
				if (point[1].m_p.x < sector[0].m_p.x)
					if (point[1].m_p.y < sector[0].m_p.y)
						if (point[1].m_p.z < sector[0].m_p.z)
							if (point[2].m_p.x < sector[0].m_p.x)
								if (point[2].m_p.y < sector[0].m_p.y)
									if (point[2].m_p.z < sector[0].m_p.z)
									{
										return true;
									}

	if (point[0].m_p.x > sector[6].m_p.x)
		if (point[0].m_p.y > sector[6].m_p.y)
			if (point[0].m_p.z > sector[6].m_p.z)
				if (point[1].m_p.x > sector[6].m_p.x)
					if (point[1].m_p.y > sector[6].m_p.y)
						if (point[1].m_p.z > sector[6].m_p.z)
							if (point[2].m_p.x > sector[6].m_p.x)
								if (point[2].m_p.y > sector[6].m_p.y)
									if (point[2].m_p.z > sector[6].m_p.z)
									{
										return true;
									}

	return false;
}

/*
*/
bool CheckPointInSector(int x, int y, int z, CVec3f* pp, CVec3f* n, CVec3f* sectorPoint)
{
	for (int vp = 0; vp < 3; vp++)
	{
		CVec3i sector = m_sector->GetSector(&pp[vp].m_p);

		if ((sector.m_p.x == x) && (sector.m_p.y == y) && (sector.m_p.z == z))
		{
			WriteCollisionPrimitive("p ", x, y, z, pp, n);

			return true;
		}
	}

	if (pp[0].m_p.x <= sectorPoint[6].m_p.x)
		if (pp[0].m_p.y <= sectorPoint[6].m_p.y)
			if (pp[0].m_p.z <= sectorPoint[6].m_p.z)
				if (pp[0].m_p.x >= sectorPoint[0].m_p.x)
					if (pp[0].m_p.y >= sectorPoint[0].m_p.y)
						if (pp[0].m_p.z >= sectorPoint[0].m_p.z)
						{
							WriteCollisionPrimitive("p1", x, y, z, pp, n);

							return true;
						}

	if (pp[1].m_p.x <= sectorPoint[6].m_p.x)
		if (pp[1].m_p.y <= sectorPoint[6].m_p.y)
			if (pp[1].m_p.z <= sectorPoint[6].m_p.z)
				if (pp[1].m_p.x >= sectorPoint[0].m_p.x)
					if (pp[1].m_p.y >= sectorPoint[0].m_p.y)
						if (pp[1].m_p.z >= sectorPoint[0].m_p.z)
						{
							WriteCollisionPrimitive("p2", x, y, z, pp, n);

							return true;
						}

	if (pp[2].m_p.x <= sectorPoint[6].m_p.x)
		if (pp[2].m_p.y <= sectorPoint[6].m_p.y)
			if (pp[2].m_p.z <= sectorPoint[6].m_p.z)
				if (pp[2].m_p.x >= sectorPoint[0].m_p.x)
					if (pp[2].m_p.y >= sectorPoint[0].m_p.y)
						if (pp[2].m_p.z >= sectorPoint[0].m_p.z)
						{
							WriteCollisionPrimitive("p3", x, y, z, pp, n);

							return true;
						}

	return false;
}

/*
*/
bool CheckTriangleRayTrace(int x, int y, int z, CVec3f* sectorPoint, CVec3f* pp, CVec3f* n)
{
	// bottom
	m_sectorTriangle[0][0] = sectorPoint[0];
	m_sectorTriangle[0][1] = sectorPoint[1];
	m_sectorTriangle[0][2] = sectorPoint[2];
	m_sectorNormal[0] = CVec3f::Normal(&sectorPoint[0], &sectorPoint[1], &sectorPoint[2]);

	m_sectorTriangle[1][0] = sectorPoint[2];
	m_sectorTriangle[1][1] = sectorPoint[3];
	m_sectorTriangle[1][2] = sectorPoint[0];
	m_sectorNormal[1] = m_sectorNormal[0];

	// top
	m_sectorTriangle[2][0] = sectorPoint[6];
	m_sectorTriangle[2][1] = sectorPoint[5];
	m_sectorTriangle[2][2] = sectorPoint[4];
	m_sectorNormal[2] = CVec3f::Normal(&sectorPoint[6], &sectorPoint[5], &sectorPoint[4]);

	m_sectorTriangle[3][0] = sectorPoint[4];
	m_sectorTriangle[3][1] = sectorPoint[7];
	m_sectorTriangle[3][2] = sectorPoint[6];
	m_sectorNormal[3] = m_sectorNormal[2];

	// front
	m_sectorTriangle[4][0] = sectorPoint[7];
	m_sectorTriangle[4][1] = sectorPoint[4];
	m_sectorTriangle[4][2] = sectorPoint[0];
	m_sectorNormal[4] = CVec3f::Normal(&sectorPoint[7], &sectorPoint[4], &sectorPoint[0]);

	m_sectorTriangle[5][0] = sectorPoint[0];
	m_sectorTriangle[5][1] = sectorPoint[3];
	m_sectorTriangle[5][2] = sectorPoint[7];
	m_sectorNormal[5] = m_sectorNormal[4];

	// right
	m_sectorTriangle[6][0] = sectorPoint[6];
	m_sectorTriangle[6][1] = sectorPoint[7];
	m_sectorTriangle[6][2] = sectorPoint[3];
	m_sectorNormal[6] = CVec3f::Normal(&sectorPoint[6], &sectorPoint[7], &sectorPoint[3]);

	m_sectorTriangle[7][0] = sectorPoint[3];
	m_sectorTriangle[7][1] = sectorPoint[2];
	m_sectorTriangle[7][2] = sectorPoint[6];
	m_sectorNormal[7] = m_sectorNormal[6];

	// back
	m_sectorTriangle[8][0] = sectorPoint[0];
	m_sectorTriangle[8][1] = sectorPoint[4];
	m_sectorTriangle[8][2] = sectorPoint[7];
	m_sectorNormal[8] = CVec3f::Normal(&sectorPoint[0], &sectorPoint[4], &sectorPoint[7]);

	m_sectorTriangle[9][0] = sectorPoint[7];
	m_sectorTriangle[9][1] = sectorPoint[3];
	m_sectorTriangle[9][2] = sectorPoint[0];
	m_sectorNormal[9] = m_sectorNormal[8];

	// left
	m_sectorTriangle[10][0] = sectorPoint[0];
	m_sectorTriangle[10][1] = sectorPoint[4];
	m_sectorTriangle[10][2] = sectorPoint[5];
	m_sectorNormal[10] = CVec3f::Normal(&sectorPoint[0], &sectorPoint[4], &sectorPoint[5]);

	m_sectorTriangle[11][0] = sectorPoint[5];
	m_sectorTriangle[11][1] = sectorPoint[1];
	m_sectorTriangle[11][2] = sectorPoint[0];
	m_sectorNormal[11] = m_sectorNormal[10];

	for (int t = 0; t < 3; t++)
	{
		for (int i = 0; i < 12; i++)
		{
			int p = t + 1;

			if (p == 3) p = 0;

			m_dir = pp[p] - pp[t];

			float length = m_dir.Length();

			m_dir.Normalize();

			if (m_dir.Dot(&m_sectorNormal[i]) > 0.0f)
			{
				continue;
			}

			m_collision.IntersectPlane(&m_sectorNormal[i], &m_sectorTriangle[i][0], &pp[t], &m_dir);

			if (m_collision.m_length < 0.0f)
			{
				continue;
			}

			if (m_collision.m_length > length)
			{
				continue;
			}

			if (m_collision.PointInTriangle(pp[t], m_sectorTriangle[i][0], m_sectorTriangle[i][1], m_sectorTriangle[i][2]))
			{
				WriteCollisionPrimitive("d ", x, y, z, pp, n);

				return true;
			}
		}
	}

	return false;
}

/*
*/
bool CheckSectorRayTrace(int x, int y, int z, CVec3f* sectorPoint, CVec3f* pp, CVec3f* n)
{
	for (int sp = 0; sp < 3; sp++)
	{
		m_dir = sectorPoint[sp + 1] - sectorPoint[sp];

		m_dir.Normalize();

		if (m_dir.Dot(n) > 0.0f)
		{
			continue;
		}

		m_collision.IntersectPlane(n, &pp[0], &sectorPoint[sp], &m_dir);

		if (m_collision.m_length < 0.0f)
		{
			continue;
		}

		if (m_collision.m_length > m_sector->m_sectorSize)
		{
			continue;
		}

		if (m_collision.PointInTriangle(sectorPoint[sp], pp[0], pp[1], pp[2]))
		{
			WriteCollisionPrimitive("a ", x, y, z, pp, n);

			return true;
		}
	}

	m_dir = sectorPoint[0] - sectorPoint[3];

	m_dir.Normalize();

	if (m_dir.Dot(n) > 0.0f)
	{
	}
	else
	{
		m_collision.IntersectPlane(n, &pp[0], &sectorPoint[3], &m_dir);

		if ((m_collision.m_length >= 0.0f) && (m_collision.m_length <= m_sector->m_sectorSize))
		{
			if (m_collision.PointInTriangle(sectorPoint[3], pp[0], pp[1], pp[2]))
			{
				WriteCollisionPrimitive("a1", x, y, z, pp, n);

				return true;
			}
		}
	}

	for (int sp = 4; sp < 7; sp++)
	{
		m_dir = sectorPoint[sp + 1] - sectorPoint[sp];

		m_dir.Normalize();

		if (m_dir.Dot(n) > 0.0f)
		{
			continue;
		}

		m_collision.IntersectPlane(n, &pp[0], &sectorPoint[sp], &m_dir);

		if (m_collision.m_length < 0.0f)
		{
			continue;
		}

		if (m_collision.m_length > m_sector->m_sectorSize)
		{
			continue;
		}

		if (m_collision.PointInTriangle(sectorPoint[sp], pp[0], pp[1], pp[2]))
		{
			WriteCollisionPrimitive("a2", x, y, z, pp, n);

			return true;
		}
	}

	m_dir = sectorPoint[4] - sectorPoint[7];

	m_dir.Normalize();

	if (m_dir.Dot(n) > 0.0f)
	{
	}
	else
	{
		m_collision.IntersectPlane(n, &pp[0], &sectorPoint[7], &m_dir);

		if ((m_collision.m_length >= 0.0f) && (m_collision.m_length <= m_sector->m_sectorSize))
		{
			if (m_collision.PointInTriangle(sectorPoint[7], pp[0], pp[1], pp[2]))
			{
				WriteCollisionPrimitive("a3", x, y, z, pp, n);

				return true;
			}
		}
	}

	for (int sp = 0; sp < 4; sp++)
	{
		m_dir = sectorPoint[sp] - sectorPoint[sp + 4];

		m_dir.Normalize();

		if (m_dir.Dot(n) > 0.0f)
		{
			continue;
		}

		m_collision.IntersectPlane(n, &pp[0], &sectorPoint[sp + 4], &m_dir);

		if (m_collision.m_length < 0.0f)
		{
			continue;
		}

		if (m_collision.m_length > m_sector->m_sectorSize)
		{
			continue;
		}

		if (m_collision.PointInTriangle(sectorPoint[sp + 4], pp[0], pp[1], pp[2]))
		{
			WriteCollisionPrimitive("b ", x, y, z, pp, n);

			return true;
		}
	}

	return false;
}

/*
*/
void WriteCollisionPrimitive(const char* t, int x, int y, int z, CVec3f* pp, CVec3f* n)
{
	/*
	printf("%s %03i %03i %03i %f %f %f %f %f %f %f %f %f %f %f %f\n",
		t,
		x,
		y,
		z,
		pp[0].m_p.x, pp[0].m_p.y, pp[0].m_p.z,
		pp[1].m_p.x, pp[1].m_p.y, pp[1].m_p.z,
		pp[2].m_p.x, pp[2].m_p.y, pp[2].m_p.z,
		n->m_p.x, n->m_p.y, n->m_p.z);
	*/
	fwrite(&x, sizeof(int), 1, m_fCollision);
	fwrite(&y, sizeof(int), 1, m_fCollision);
	fwrite(&z, sizeof(int), 1, m_fCollision);

	fwrite(pp, sizeof(CVec3f), 3, m_fCollision);

	fwrite(n, sizeof(CVec3f), 1, m_fCollision);
}

/*
*/
void CreateTerrain(CEntity* entity)
{
	printf("\nCreating Terrain\n");

	char* name = {};

	entity->GetKeyValue("map", &name);
	entity->GetKeyValue("tileSize", &primSize);
	entity->GetKeyValue("vheight", &vheight);
	entity->GetKeyValue("width", &width);
	entity->GetKeyValue("height", &height);
	entity->GetKeyValue("origin", &m_origin);

	float t = m_origin.m_p.y;
	m_origin.m_p.y = m_origin.m_p.z;
	m_origin.m_p.z = t;


	CString* filename = new CString(m_local.m_installPath->m_text);

	filename->Append(m_local.m_assetDirectory->m_text);
	filename->Append(name);

	if (filename->Search(".bmp"))
	{
		bmp = new CBmpImage(filename->m_text);

		if (!bmp->m_isInitialized)
		{
			printf("\nTerrain not found:%s\n", filename->m_text);

			return;
		}
	}

	printf("\nBuilding Vertices\n");


	if (bmp != nullptr)
	{
		width = bmp->m_bmapInfo.bmiHeader.biWidth;
		height = bmp->m_bmapInfo.bmiHeader.biHeight;

		uinc = 1.0f / width;
		vinc = 1.0f / height;

		u = 0.0f;
		v = 1.0f;

		pixelStart = (bmp->m_pixels8 + (size_t)width * height * bmp->m_bytesPerPixel) - width;

		x = ((float)width / 2) * -primSize;
		z = ((float)height / 2) * -primSize;

		verts = new CHeapArray(sizeof(CVec3f), true, false, 2, width, height);

		for (int h = 0; h < height; h++)
		{
			x = ((float)width / 2) * -primSize;

			for (int w = 0; w < width; w++)
			{
#ifdef _DEBUG
				printf("H:%03iW:%03i\r", h + 1, w + 1);
#endif
				CVec3f* vertex = (CVec3f*)verts->GetElement(2, w, h);

				vertex->m_p.x = x + m_origin.m_p.x;
				vertex->m_p.y = (float)(bmp->m_paletteentry[*pixelStart].peRed * vheight) + m_origin.m_p.y;
				vertex->m_p.z = z + m_origin.m_p.z;

				vertexCount++;

				x += primSize;

				pixelStart++;
			}

			z += primSize;

			pixelStart -= ((int)width * 2);
		}
	}
	else
	{
		uinc = 1.0f / width;
		vinc = 1.0f / height;

		u = 0.0f;
		v = 1.0f;


		m_objectScript.InitBuffer(filename->m_text);

		if (m_objectScript.CheckEndOfBuffer())
		{
			printf("\nTerrain not found:%s\n", filename->m_text);
			
			return;
		}

		verts = new CHeapArray(sizeof(CVec3f), true, false, 2, width, height);

		for (int h = 0; h < height; h++)
		{
			for (int w = 0; w < width; w++)
			{
#ifdef _DEBUG
				printf("H:%03iW:%03i\r", h + 1, w + 1);
#endif

				CVec3f* vertex = (CVec3f*)verts->GetElement(2, w, h);

				if (m_objectScript.CheckEndOfBuffer())
				{
					break;
				}

				m_objectScript.MoveToToken("v");

				m_objectScript.Move(2);

				// need to flip z and y when blender export
				vertex->m_p.x = strtof(m_objectScript.GetToken(), nullptr);
				vertex->m_p.z = strtof(m_objectScript.GetToken(), nullptr);
				vertex->m_p.y = strtof(m_objectScript.GetToken(), nullptr);

				// origin was already flipped
				vertex->m_p.x += m_origin.m_p.x;
				vertex->m_p.y += m_origin.m_p.y;
				vertex->m_p.z += m_origin.m_p.z;

				m_objectScript.SkipEndOfLine();

				vertexCount++;
			}
		}
	}

	delete filename;


	m_err = fopen_s(&master, "master.txt", "wb");

	if (m_err)
	{
		printf("Error opening master.txt wb\n");

		delete verts;
		delete bmp;

		return;
	}


	/*
	Ground Collision

	B  E-F
	|\  \|
	A-C  D

	BAC is N1
	DFE is N2
	*/

	printf("\nWriting Collision\n");

	for (int h = 0; h < height - 1; h++)
	{
		for (int w = 0; w < width - 1; w++)
		{
#ifdef _DEBUG
			printf("H:%03iW:%03i\r", h + 1, w + 1);
#endif

			CVec3f* A = (CVec3f*)verts->GetElement(2, w, h);
			CVec3f* B = (CVec3f*)verts->GetElement(2, w, h + 1);
			CVec3f* C = (CVec3f*)verts->GetElement(2, w + 1, h);

			CVec3f n1 = n1.Normal(B, A, C);

			CVec3f* D = (CVec3f*)verts->GetElement(2, w + 1, h);
			CVec3f* E = (CVec3f*)verts->GetElement(2, w, h + 1);
			CVec3f* F = (CVec3f*)verts->GetElement(2, w + 1, h + 1);

			CVec3f n2 = n2.Normal(D, F, E);

			WriteTile(C, A, B, &n1, E, F, D, &n2, true, u, v);


			CVec3f ct[4] = {};

			memcpy(&ct[0], A, sizeof(CVec3f));
			memcpy(&ct[1], B, sizeof(CVec3f));
			memcpy(&ct[2], C, sizeof(CVec3f));
			memcpy(&ct[3], &n1, sizeof(CVec3f));

			WriteCollisionPrimitiveToSectors(ct);


			memcpy(&ct[0], D, sizeof(CVec3f));
			memcpy(&ct[1], E, sizeof(CVec3f));
			memcpy(&ct[2], F, sizeof(CVec3f));
			memcpy(&ct[3], &n2, sizeof(CVec3f));

			WriteCollisionPrimitiveToSectors(ct);


			u += uinc;
		}

		u = 0.0f;

		v -= vinc;
	}

	fclose(master);




	printf("\nAveraging Vertex Normals\n");

	m_err = fopen_s(&master, "master.txt", "rb+");

	if (m_err)
	{
		printf("\nError opening master.txt rb+\n");

		return;
	}

	tiles = new CHeapArray(sizeof(CTerrainTile), false, false, 2, width - 1, height - 1);

	for (int h = 0; h < height - 1; h++)
	{
		for (int w = 0; w < width - 1; w++)
		{
#ifdef _DEBUG
			printf("H:%03iW:%03i\r", h + 1, w + 1);
#endif

			int w1 = ((w - 1) < 0) ? 0 : w - 1;
			int h1 = ((h - 1) < 0) ? 0 : h - 1;

			int w1h1 = tiles->GetOffset(2, w1, h1);

			fseek(master, w1h1, SEEK_SET);
			fread(&t1, sizeof(CTerrainTile), 1, master);


			int w2 = ((w - 1) < 0) ? 0 : w - 1;
			int h2 = h;

			int w2h2 = tiles->GetOffset(2, w2, h2);

			fseek(master, w2h2, SEEK_SET);
			fread(&t2, sizeof(CTerrainTile), 1, master);


			int w3 = w;
			int h3 = h;

			int w3h3 = tiles->GetOffset(2, w3, h3);

			fseek(master, w3h3, SEEK_SET);
			fread(&t3, sizeof(CTerrainTile), 1, master);


			int w4 = w;
			int h4 = ((h - 1) < 0) ? 0 : h - 1;

			int w4h4 = tiles->GetOffset(2, w4, h4);

			fseek(master, w4h4, SEEK_SET);
			fread(&t4, sizeof(CTerrainTile), 1, master);


			float sumX = t1.m_f.m_n.x + t2.m_c.m_n.x + t2.m_d.m_n.x + t3.m_a.m_n.x + t4.m_e.m_n.x + t4.m_b.m_n.x;
			float sumY = t1.m_f.m_n.y + t2.m_c.m_n.y + t2.m_d.m_n.y + t3.m_a.m_n.y + t4.m_e.m_n.y + t4.m_b.m_n.y;
			float sumZ = t1.m_f.m_n.z + t2.m_c.m_n.z + t2.m_d.m_n.z + t3.m_a.m_n.z + t4.m_e.m_n.z + t4.m_b.m_n.z;

			float aX = sumX / 6.0f;
			float aY = sumY / 6.0f;
			float aZ = sumZ / 6.0f;

			CVec3f nvn = CVec3f(aX, aY, aZ);

			nvn.Normalize();

			t1.m_f.m_n.x = t2.m_c.m_n.x = t2.m_d.m_n.x = t3.m_a.m_n.x = t4.m_e.m_n.x = t4.m_b.m_n.x = nvn.m_p.x;
			t1.m_f.m_n.y = t2.m_c.m_n.y = t2.m_d.m_n.y = t3.m_a.m_n.y = t4.m_e.m_n.y = t4.m_b.m_n.y = nvn.m_p.y;
			t1.m_f.m_n.z = t2.m_c.m_n.z = t2.m_d.m_n.z = t3.m_a.m_n.z = t4.m_e.m_n.z = t4.m_b.m_n.z = nvn.m_p.z;


			fseek(master, w1h1, SEEK_SET);
			fwrite(&t1, sizeof(CTerrainTile), 1, master);

			fseek(master, w2h2, SEEK_SET);
			fwrite(&t2, sizeof(CTerrainTile), 1, master);

			fseek(master, w3h3, SEEK_SET);
			fwrite(&t3, sizeof(CTerrainTile), 1, master);

			fseek(master, w4h4, SEEK_SET);
			fwrite(&t4, sizeof(CTerrainTile), 1, master);
		}
	}

	fclose(master);

	delete tiles;




	printf("\nWriting Vertices from tiles\n");

	m_err = fopen_s(&master, "master.txt", "rb");

	if (m_err)
	{
		printf("\nError opening master.txt rb\n");

		return;
	}

	entity->GetKeyValue("compile", &name);

	
	filename = new CString(m_local.m_installPath->m_text);

	filename->Append(m_local.m_assetDirectory->m_text);
	filename->Append(name);

	m_err = fopen_s(&verticesFile, filename->m_text, "wb");

	if (verticesFile == 0)
	{
		return;
	}

	tiles = new CHeapArray(sizeof(CTerrainTile), false, false, 2, width - 1, height - 1);

	int c = 0;

	for (int h = 0; h < height - 1; h++)
	{
		for (int w = 0; w < width - 1; w++)
		{
#ifdef _DEBUG
			printf("H:%03iW:%03i\r", h + 1, w + 1);
#endif

			int wh = tiles->GetOffset(2, w, h);

			fseek(master, wh, SEEK_SET);
			fread(&t1, sizeof(CTerrainTile), 1, master);

			fwrite(&t1.m_a, sizeof(CVertexNT), 1, verticesFile);

			c++;
		}

		fwrite(&t1.m_c, sizeof(CVertexNT), 1, verticesFile);

		c++;
	}

	for (int w = 0; w < width - 1; w++)
	{
		// height - 2 for last tile
		int wh = tiles->GetOffset(2, w, height - 2);

		fseek(master, wh, SEEK_SET);
		fread(&t1, sizeof(CTerrainTile), 1, master);

		fwrite(&t1.m_b, sizeof(CVertexNT), 1, verticesFile);

		c++;
	}

	fwrite(&t1.m_f, sizeof(CVertexNT), 1, verticesFile);

	c++;


	if (master)
	{
		fclose(master);
	}

	if (verticesFile)
	{
		fclose(verticesFile);
	}

	delete tiles;

	delete filename;





	printf("\nWriting Blend Masks\n");

	m_err = fopen_s(&master, "master.txt", "rb");

	if (m_err)
	{
		return;
	}


	entity->GetKeyValue("texture0", &name);

	int x = {};
	int y = {};
	char imageName[128] = {};

	sscanf_s(name, "%i %i %s", &x, &y, imageName, 128);


	filename = new CString(m_local.m_installPath->m_text);

	filename->Append(m_local.m_assetDirectory->m_text);
	filename->Append(imageName);


	m_err = fopen_s(&maskFile, filename->m_text, "wb");

	if (m_err)
	{
		return;
	}

	mask = new CBmpImage();
	mask->WriteBitmapHeader(width, height, maskFile);
	mask->WriteColorPalette(maskFile);

	delete filename;


	entity->GetKeyValue("texture1", &name);

	sscanf_s(name, "%i %i %s", &x, &y, imageName, 128);

	filename = new CString(m_local.m_installPath->m_text);

	filename->Append(m_local.m_assetDirectory->m_text);
	filename->Append(imageName);

	m_err = fopen_s(&lightmapFile, filename->m_text, "wb");

	if (m_err)
	{
		return;
	}

	lightmap = new CBmpImage();
	lightmap->WriteBitmapHeader(width, height, lightmapFile);
	lightmap->WriteGreyscalePalette(lightmapFile);

	tiles = new CHeapArray(sizeof(CTerrainTile), false, true, 2, width - 1, height - 1);

	BYTE p = 0;

	for (int h = 0; h < height - 1; h++)
	{
		for (int w = 0; w < width - 1; w++)
		{
#ifdef _DEBUG
			printf("H:%03iW:%03i\r", h + 1, w + 1);
#endif

			int w1h1 = tiles->GetOffset(2, w, h);

			fseek(master, w1h1, SEEK_SET);
			fread(&t1, sizeof(CTerrainTile), 1, master);

			CVec3f n = CVec3f(t1.m_a.m_n.x, t1.m_a.m_n.y, t1.m_a.m_n.z);

			n.RadiusNormalize();

			float NdL = n.Dot(&down);

			float NdLl = n.Dot(&down);

			p = (BYTE)(255.0f * fabsf(NdLl));
			fwrite(&p, sizeof(BYTE), 1, lightmapFile);

			p = 0;

			if (NdL < -0.85)
			{
				p = 0;
			}
			else if (NdL < -0.75)
			{
				p = 1;
			}
			else if (NdL < -0.65)
			{
				p = 2;
			}
			else if (NdL < -0.45)
			{
				p = 3;
			}

			fwrite(&p, sizeof(BYTE), 1, maskFile);
		}

		p = 0;

		fwrite(&p, sizeof(BYTE), 1, maskFile);
		fwrite(&p, sizeof(BYTE), 1, lightmapFile);
	}

	p = 0;

	for (int w = 0; w < width; w++)
	{
		fwrite(&p, sizeof(BYTE), 1, maskFile);
		fwrite(&p, sizeof(BYTE), 1, lightmapFile);
	}

	if (master)
	{
		fclose(master);
	}

	if (maskFile)
	{
		fclose(maskFile);
	}

	if (lightmapFile)
	{
		fclose(lightmapFile);
	}

	delete filename;


	printf("\nnumber of vertices:%i\n", c);


}

/*
*/
void WriteTile(CVec3f* A, CVec3f* B, CVec3f* C, CVec3f* n1, CVec3f* D, CVec3f* E, CVec3f* F, CVec3f* n2, bool uuv, float tu, float tv)
{
	fwrite(A, sizeof(CVec3f), 1, master);
	WriteNormal(n1);

	if (uuv)
	{
		WriteUVCoord(tu, tv);
	}
	else
	{
		WriteUVCoord(0.0f, 1.0f);
	}

	fwrite(B, sizeof(CVec3f), 1, master);
	WriteNormal(n1);

	if (uuv)
	{
		WriteUVCoord(tu, tv - vinc);
	}
	else
	{
		WriteUVCoord(0.0f, 0.0f);
	}


	fwrite(C, sizeof(CVec3f), 1, master);
	WriteNormal(n1);

	if (uuv)
	{
		WriteUVCoord(tu + uinc, tv);
	}
	else
	{
		WriteUVCoord(1.0f, 1.0f);
	}



	fwrite(D, sizeof(CVec3f), 1, master);
	WriteNormal(n2);

	if (uuv)
	{
		WriteUVCoord(tu + uinc, tv);
	}
	else
	{
		WriteUVCoord(1.0f, 1.0f);
	}


	fwrite(E, sizeof(CVec3f), 1, master);
	WriteNormal(n2);

	if (uuv)
	{
		WriteUVCoord(tu, tv - vinc);
	}
	else
	{
		WriteUVCoord(0.0f, 0.0f);
	}


	fwrite(F, sizeof(CVec3f), 1, master);
	WriteNormal(n2);

	if (uuv)
	{
		WriteUVCoord(tu + uinc, tv - vinc);
	}
	else
	{
		WriteUVCoord(1.0f, 0.0f);
	}
}

/*
*/
void WriteNormal(CVec3f* n)
{
	fwrite(n, sizeof(CVec3f), 1, master);
}

/*
*/
void WriteUVCoord(float tu, float tv)
{
	fwrite(&tu, sizeof(float), 1, master);
	fwrite(&tv, sizeof(float), 1, master);
}