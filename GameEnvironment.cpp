#include "../GameCommon/framework.h"

#include "../GameCommon/CBmpImage.h"
#include "../GameCommon/CBrush.h"
#include "../GameCommon/CCollision.h"
#include "../GameCommon/CCollisionPrimitive.h"
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

bool m_wroteHeader[MAX_MATERIALS];

CCollision m_collision;
CDirectoryList* m_directoryList;
CEntity m_entity[MAX_ENTITIES];

char m_collisionDirectory[LONG_STRING] = {};
char m_directory[LONG_STRING] = {};
char m_key[SHORT_STRING] = {};
char m_mapDirectory[LONG_STRING] = {};
char m_mapName[LONG_STRING] = {};
char m_mapShortName[SHORT_STRING] = {};
char m_shaderDirectory[LONG_STRING] = {};
char m_textureDirectory[LONG_STRING] = {};
char m_value[LONG_STRING] = {};

char* m_surface;

CHeap* m_heapCollision[MAX_MATERIALS];
CHeap* m_heapMap[MAX_MATERIALS];
CLocal m_local;
CSector* m_sector;
CShaderMaterial m_masterMaterials[MAX_MATERIALS];
CShaderMaterial m_materials[MAX_MATERIALS];
CTgaImage m_images[MAX_MATERIALS];
CToken m_mapScript;
CToken m_objectScript;
CToken m_shaderScript;
CVec3f m_dir;
CVec3f m_sectorNormal[12];
CVec3f m_sectorTriangle[12][3];
CVec3i m_mapSize;

errno_t m_err;

FILE* m_fCollision;
FILE* m_fMap;

int m_brushCount = 0;
int m_entityCollisionCount = 0;
int m_entityCount = 0;
int m_entityMapCount = 0;
int m_masterMaterialsCount = 0;
int m_materialsCount = 0;
int m_sectorSize;





BYTE* m_pixelStart;

CBmpImage* m_heightmapImage;
CBmpImage* m_lightmapImage;
CBmpImage* m_maskImage;

char m_textureName[LONG_STRING] = {};

CHeapArray* m_tiles;
CHeapArray* m_vertices;
CString* m_terrainFilename;
CString* m_heightmapFilename;
CString* m_lightmapFilename;
CString* m_maskFilename;
CTerrainTile m_terrainTile1;
CTerrainTile m_terrainTile2;
CTerrainTile m_terrainTile3;
CTerrainTile m_terrainTile4;
CVec3f m_down = CVec3f(0.0f, -1.0f, 0.0f);
CVec3f m_origin;

FILE* lightmapFile;
FILE* maskFile;
FILE* master;
FILE* verticesFile;

float m_originTemp;
float m_u;
float m_uIncrement;
float m_v;
float m_vheight;
float m_vIncrement;
float m_xIncrement;
float m_zIncrement;

int m_primSize;
int m_terrainHeight;
int m_terrainWidth;
int m_vertexCount;
int m_xRepeat;
int m_yRepeat;







void LoadAllShaders();
void ParseShaderScript();
void WriteMaterials();

bool CheckTriangleOutsideOfSector(CCollisionPrimitive* collisionPrimitive, CVec3f* sector);
bool CheckPointInSector(int x, int y, int z, CCollisionPrimitive* collisionPrimitive, CVec3f* sectorPoint);
bool CheckTriangleRayTrace(int x, int y, int z, CCollisionPrimitive* collisionPrimitive, CVec3f* sectorPoint);
bool CheckSectorRayTrace(int x, int y, int z, CCollisionPrimitive* collisionPrimitive, CVec3f* sectorPoint);
void WriteBrush(CBrush* brush);
void WriteCollisionPrimitiveToSectors(CCollisionPrimitive* collisionPrimitive);

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

			m_entity[m_entityCount].Constructor(NULL);

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

	CLinkListNode<CString>* shader = m_directoryList->m_files->m_list;

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

					m_surface = m_shaderScript.GetToken();

					m_masterMaterials[m_masterMaterialsCount].SetSurface(m_surface, 1);

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

						m_heapCollision[i]->Append((unsigned char*)&brush->m_brushSide[s].m_shaderMaterial->m_surface, sizeof(uint32_t));
						m_heapCollision[i]->Append((unsigned char*)&brush->m_brushSide[s].m_center.m_v.m_p, sizeof(CVec3f));

						if (v == (brush->m_brushSide[s].m_vertexCount - 1))
						{
							m_heapMap[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v].m_v, sizeof(CVertexNT));
							m_heapMap[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[0].m_v, sizeof(CVertexNT));

							m_heapCollision[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v].m_v.m_p, sizeof(CVec3f));
							m_heapCollision[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[0].m_v.m_p, sizeof(CVec3f));
						}
						else
						{
							m_heapMap[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v].m_v, sizeof(CVertexNT));
							m_heapMap[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v + 1].m_v, sizeof(CVertexNT));

							m_heapCollision[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v].m_v.m_p, sizeof(CVec3f));
							m_heapCollision[i]->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v + 1].m_v.m_p, sizeof(CVec3f));
						}

						m_heapCollision[i]->Append((unsigned char*)&brush->m_brushSide[s].m_center.m_v.m_n, sizeof(CVec3f));
					}
				}
			}
		}
	}

	for (int i = 0; i < m_materialsCount; i++)
	{
		if (m_heapMap[i])
		{
			fwrite(m_heapMap[i]->m_heap, m_heapMap[i]->m_length, 1, m_fMap);

			delete m_heapMap[i];

			m_heapMap[i] = nullptr;
		}

		if (m_heapCollision[i])
		{
			CCollisionPrimitive* collisionPrimitive = (CCollisionPrimitive*)m_heapCollision[i]->m_heap;

			int count = m_heapCollision[i]->m_length / sizeof(CCollisionPrimitive);

			for (int ii = 0;ii < count;ii++)
			{
				WriteCollisionPrimitiveToSectors(&collisionPrimitive[ii]);
			}

			delete m_heapCollision[i];

			m_heapCollision[i] = nullptr;
		}
	}
}

/*
*/
void WriteCollisionPrimitiveToSectors(CCollisionPrimitive* collisionPrimitive)
{
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
				if (CheckTriangleOutsideOfSector(collisionPrimitive, sectorVolume))
				{
				}
				else if (CheckPointInSector(x, y, z, collisionPrimitive, sectorVolume))
				{
				}
				else if (CheckSectorRayTrace(x, y, z, collisionPrimitive, sectorVolume))
				{
				}
				else
				{
					CheckTriangleRayTrace(x, y, z, collisionPrimitive, sectorVolume);
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
bool CheckTriangleOutsideOfSector(CCollisionPrimitive* collisionPrimitive, CVec3f* sector)
{
	if (collisionPrimitive->m_a.m_p.x < sector[0].m_p.x)
		if (collisionPrimitive->m_a.m_p.y < sector[0].m_p.y)
			if (collisionPrimitive->m_a.m_p.z < sector[0].m_p.z)
				if (collisionPrimitive->m_b.m_p.x < sector[0].m_p.x)
					if (collisionPrimitive->m_b.m_p.y < sector[0].m_p.y)
						if (collisionPrimitive->m_b.m_p.z < sector[0].m_p.z)
							if (collisionPrimitive->m_c.m_p.x < sector[0].m_p.x)
								if (collisionPrimitive->m_c.m_p.y < sector[0].m_p.y)
									if (collisionPrimitive->m_c.m_p.z < sector[0].m_p.z)
									{
										return true;
									}

	if (collisionPrimitive->m_a.m_p.x > sector[6].m_p.x)
		if (collisionPrimitive->m_a.m_p.y > sector[6].m_p.y)
			if (collisionPrimitive->m_a.m_p.z > sector[6].m_p.z)
				if (collisionPrimitive->m_b.m_p.x > sector[6].m_p.x)
					if (collisionPrimitive->m_b.m_p.y > sector[6].m_p.y)
						if (collisionPrimitive->m_b.m_p.z > sector[6].m_p.z)
							if (collisionPrimitive->m_c.m_p.x > sector[6].m_p.x)
								if (collisionPrimitive->m_c.m_p.y > sector[6].m_p.y)
									if (collisionPrimitive->m_c.m_p.z > sector[6].m_p.z)
									{
										return true;
									}

	return false;
}

/*
*/
bool CheckPointInSector(int x, int y, int z, CCollisionPrimitive* collisionPrimitive, CVec3f* sectorPoint)
{
	CVec3f* pp = &collisionPrimitive->m_a;

	for (int vp = 0; vp < 3; vp++)
	{
		CVec3i sector = m_sector->GetSector(&pp[vp].m_p);

		if ((sector.m_p.x == x) && (sector.m_p.y == y) && (sector.m_p.z == z))
		{
			fwrite(&x, sizeof(int), 1, m_fCollision);
			fwrite(&y, sizeof(int), 1, m_fCollision);
			fwrite(&z, sizeof(int), 1, m_fCollision);

			collisionPrimitive->WritePrimitive(m_fCollision);

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
							fwrite(&x, sizeof(int), 1, m_fCollision);
							fwrite(&y, sizeof(int), 1, m_fCollision);
							fwrite(&z, sizeof(int), 1, m_fCollision);

							collisionPrimitive->WritePrimitive(m_fCollision);

							return true;
						}

	if (pp[1].m_p.x <= sectorPoint[6].m_p.x)
		if (pp[1].m_p.y <= sectorPoint[6].m_p.y)
			if (pp[1].m_p.z <= sectorPoint[6].m_p.z)
				if (pp[1].m_p.x >= sectorPoint[0].m_p.x)
					if (pp[1].m_p.y >= sectorPoint[0].m_p.y)
						if (pp[1].m_p.z >= sectorPoint[0].m_p.z)
						{
							fwrite(&x, sizeof(int), 1, m_fCollision);
							fwrite(&y, sizeof(int), 1, m_fCollision);
							fwrite(&z, sizeof(int), 1, m_fCollision);

							collisionPrimitive->WritePrimitive(m_fCollision);

							return true;
						}

	if (pp[2].m_p.x <= sectorPoint[6].m_p.x)
		if (pp[2].m_p.y <= sectorPoint[6].m_p.y)
			if (pp[2].m_p.z <= sectorPoint[6].m_p.z)
				if (pp[2].m_p.x >= sectorPoint[0].m_p.x)
					if (pp[2].m_p.y >= sectorPoint[0].m_p.y)
						if (pp[2].m_p.z >= sectorPoint[0].m_p.z)
						{
							fwrite(&x, sizeof(int), 1, m_fCollision);
							fwrite(&y, sizeof(int), 1, m_fCollision);
							fwrite(&z, sizeof(int), 1, m_fCollision);

							collisionPrimitive->WritePrimitive(m_fCollision);

							return true;
						}

	return false;
}

/*
*/
bool CheckTriangleRayTrace(int x, int y, int z, CCollisionPrimitive* collisionPrimitive, CVec3f* sectorPoint)
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

	CVec3f* pp = &collisionPrimitive->m_a;

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
				fwrite(&x, sizeof(int), 1, m_fCollision);
				fwrite(&y, sizeof(int), 1, m_fCollision);
				fwrite(&z, sizeof(int), 1, m_fCollision);

				collisionPrimitive->WritePrimitive(m_fCollision);

				return true;
			}
		}
	}

	return false;
}

/*
*/
bool CheckSectorRayTrace(int x, int y, int z, CCollisionPrimitive* collisionPrimitive, CVec3f* sectorPoint)
{
	CVec3f* pp = &collisionPrimitive->m_a;

	for (int sp = 0; sp < 3; sp++)
	{
		m_dir = sectorPoint[sp + 1] - sectorPoint[sp];

		m_dir.Normalize();

		if (m_dir.Dot(&collisionPrimitive->m_n) > 0.0f)
		{
			continue;
		}

		m_collision.IntersectPlane(&collisionPrimitive->m_n, &pp[0], &sectorPoint[sp], &m_dir);

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
			fwrite(&x, sizeof(int), 1, m_fCollision);
			fwrite(&y, sizeof(int), 1, m_fCollision);
			fwrite(&z, sizeof(int), 1, m_fCollision);

			collisionPrimitive->WritePrimitive(m_fCollision);

			return true;
		}
	}

	m_dir = sectorPoint[0] - sectorPoint[3];

	m_dir.Normalize();

	if (m_dir.Dot(&collisionPrimitive->m_n) > 0.0f)
	{
	}
	else
	{
		m_collision.IntersectPlane(&collisionPrimitive->m_n, &pp[0], &sectorPoint[3], &m_dir);

		if ((m_collision.m_length >= 0.0f) && (m_collision.m_length <= m_sector->m_sectorSize))
		{
			if (m_collision.PointInTriangle(sectorPoint[3], pp[0], pp[1], pp[2]))
			{
				fwrite(&x, sizeof(int), 1, m_fCollision);
				fwrite(&y, sizeof(int), 1, m_fCollision);
				fwrite(&z, sizeof(int), 1, m_fCollision);

				collisionPrimitive->WritePrimitive(m_fCollision);

				return true;
			}
		}
	}

	for (int sp = 4; sp < 7; sp++)
	{
		m_dir = sectorPoint[sp + 1] - sectorPoint[sp];

		m_dir.Normalize();

		if (m_dir.Dot(&collisionPrimitive->m_n) > 0.0f)
		{
			continue;
		}

		m_collision.IntersectPlane(&collisionPrimitive->m_n, &pp[0], &sectorPoint[sp], &m_dir);

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
			fwrite(&x, sizeof(int), 1, m_fCollision);
			fwrite(&y, sizeof(int), 1, m_fCollision);
			fwrite(&z, sizeof(int), 1, m_fCollision);

			collisionPrimitive->WritePrimitive(m_fCollision);

			return true;
		}
	}

	m_dir = sectorPoint[4] - sectorPoint[7];

	m_dir.Normalize();

	if (m_dir.Dot(&collisionPrimitive->m_n) > 0.0f)
	{
	}
	else
	{
		m_collision.IntersectPlane(&collisionPrimitive->m_n, &pp[0], &sectorPoint[7], &m_dir);

		if ((m_collision.m_length >= 0.0f) && (m_collision.m_length <= m_sector->m_sectorSize))
		{
			if (m_collision.PointInTriangle(sectorPoint[7], pp[0], pp[1], pp[2]))
			{
				fwrite(&x, sizeof(int), 1, m_fCollision);
				fwrite(&y, sizeof(int), 1, m_fCollision);
				fwrite(&z, sizeof(int), 1, m_fCollision);

				collisionPrimitive->WritePrimitive(m_fCollision);

				return true;
			}
		}
	}

	for (int sp = 0; sp < 4; sp++)
	{
		m_dir = sectorPoint[sp] - sectorPoint[sp + 4];

		m_dir.Normalize();

		if (m_dir.Dot(&collisionPrimitive->m_n) > 0.0f)
		{
			continue;
		}

		m_collision.IntersectPlane(&collisionPrimitive->m_n, &pp[0], &sectorPoint[sp + 4], &m_dir);

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
			fwrite(&x, sizeof(int), 1, m_fCollision);
			fwrite(&y, sizeof(int), 1, m_fCollision);
			fwrite(&z, sizeof(int), 1, m_fCollision);

			collisionPrimitive->WritePrimitive(m_fCollision);

			return true;
		}
	}

	return false;
}

/*
*/
void CreateTerrain(CEntity* entity)
{
	printf("\nCreating Terrain\n");

	char* name = {};

	entity->GetKeyValue("map", &name);
	entity->GetKeyValue("tileSize", &m_primSize);
	entity->GetKeyValue("vheight", &m_vheight);
	entity->GetKeyValue("width", &m_terrainWidth);
	entity->GetKeyValue("height", &m_terrainHeight);
	entity->GetKeyValue("origin", &m_origin);

	m_originTemp = m_origin.m_p.y;
	m_origin.m_p.y = m_origin.m_p.z;
	m_origin.m_p.z = m_originTemp;


	m_heightmapFilename = new CString(m_local.m_installPath->m_text);

	m_heightmapFilename->Append(m_local.m_assetDirectory->m_text);
	m_heightmapFilename->Append(name);

	if (m_heightmapFilename->Search(".bmp"))
	{
		m_heightmapImage = new CBmpImage(m_heightmapFilename->m_text);

		if (!m_heightmapImage->m_isInitialized)
		{
			printf("\nTerrain not found:%s\n", m_heightmapFilename->m_text);

			return;
		}
	}

	printf("\nBuilding Vertices\n");


	if (m_heightmapImage)
	{
		m_terrainWidth = m_heightmapImage->m_bmapInfo.bmiHeader.biWidth;
		m_terrainHeight = m_heightmapImage->m_bmapInfo.bmiHeader.biHeight;

		m_uIncrement = 1.0f / m_terrainWidth;
		m_vIncrement = 1.0f / m_terrainHeight;

		m_u = 0.0f;
		m_v = 1.0f;

		m_pixelStart = (m_heightmapImage->m_pixels8 + (size_t)m_terrainWidth * m_terrainHeight * m_heightmapImage->m_bytesPerPixel) - m_terrainWidth;

		m_xIncrement = (float)(m_terrainWidth / 2) * -m_primSize;
		m_zIncrement = (float)(m_terrainHeight / 2) * -m_primSize;

		m_vertices = new CHeapArray(true, sizeof(CVec3f), 2, m_terrainWidth, m_terrainHeight);

		for (int h = 0; h < m_terrainHeight; h++)
		{
			m_xIncrement = (float)(m_terrainWidth / 2) * -m_primSize;

			for (int w = 0; w < m_terrainWidth; w++)
			{
#ifdef _DEBUG
				printf("H:%03iW:%03i\r", h + 1, w + 1);
#endif
				CVec3f* vertex = (CVec3f*)m_vertices->GetElement(2, w, h);

				vertex->m_p.x = m_xIncrement + m_origin.m_p.x;
				vertex->m_p.y = (float)(m_heightmapImage->m_paletteEntries[*m_pixelStart].peRed * m_vheight) + m_origin.m_p.y;
				vertex->m_p.z = m_zIncrement + m_origin.m_p.z;

				m_xIncrement += m_primSize;

				m_pixelStart++;
			}

			m_zIncrement += m_primSize;

			m_pixelStart -= ((int)m_terrainWidth * 2);
		}
	}
	else
	{
		m_uIncrement = 1.0f / m_terrainWidth;
		m_vIncrement = 1.0f / m_terrainHeight;

		m_u = 0.0f;
		m_v = 1.0f;


		m_objectScript.InitBuffer(m_heightmapFilename->m_text);

		if (m_objectScript.CheckEndOfBuffer())
		{
			printf("\nTerrain not found:%s\n", m_heightmapFilename->m_text);
			
			return;
		}

		m_vertices = new CHeapArray(true, sizeof(CVec3f), 2, m_terrainWidth, m_terrainHeight);

		for (int h = 0; h < m_terrainHeight; h++)
		{
			for (int w = 0; w < m_terrainWidth; w++)
			{
#ifdef _DEBUG
				printf("H:%03iW:%03i\r", h + 1, w + 1);
#endif

				CVec3f* vertex = (CVec3f*)m_vertices->GetElement(2, w, h);

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
			}
		}
	}

	delete m_heightmapFilename;


	m_err = fopen_s(&master, "master.txt", "wb");

	if (m_err)
	{
		printf("Error opening master.txt wb\n");

		delete m_vertices;
		delete m_heightmapImage;

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

	printf("\nWriting Master\n");

	for (int h = 0; h < m_terrainHeight - 1; h++)
	{
		for (int w = 0; w < m_terrainWidth - 1; w++)
		{
#ifdef _DEBUG
			printf("H:%03iW:%03i\r", h + 1, w + 1);
#endif
			CVec3f* A = (CVec3f*)m_vertices->GetElement(2, w, h);
			CVec3f* B = (CVec3f*)m_vertices->GetElement(2, w, h + 1);
			CVec3f* C = (CVec3f*)m_vertices->GetElement(2, w + 1, h);

			CVec3f n1 = n1.Normal(B, A, C);

			CVec3f* D = (CVec3f*)m_vertices->GetElement(2, w + 1, h);
			CVec3f* E = (CVec3f*)m_vertices->GetElement(2, w, h + 1);
			CVec3f* F = (CVec3f*)m_vertices->GetElement(2, w + 1, h + 1);

			CVec3f n2 = n2.Normal(D, F, E);

			WriteTile(A, B, C, &n1, D, E, F, &n2, true, m_u, m_v);

			m_u += m_uIncrement;
		}

		m_u = 0.0f;

		m_v -= m_vIncrement;
	}

	fclose(master);




	printf("\nAveraging Vertex Normals\n");

	m_err = fopen_s(&master, "master.txt", "rb+");

	if (m_err)
	{
		printf("\nError opening master.txt rb+\n");

		return;
	}

	m_tiles = new CHeapArray(false, sizeof(CTerrainTile), 2, m_terrainWidth - 1, m_terrainHeight - 1);

	for (int h = 0; h < m_terrainHeight - 1; h++)
	{
		for (int w = 0; w < m_terrainWidth - 1; w++)
		{
#ifdef _DEBUG
			printf("H:%03iW:%03i\r", h + 1, w + 1);
#endif

			int w1 = ((w - 1) < 0) ? 0 : w - 1;
			int h1 = ((h - 1) < 0) ? 0 : h - 1;

			int w1h1 = m_tiles->GetOffset(2, w1, h1);

			fseek(master, w1h1, SEEK_SET);
			fread(&m_terrainTile1, sizeof(CTerrainTile), 1, master);


			int w2 = ((w - 1) < 0) ? 0 : w - 1;
			int h2 = h;

			int w2h2 = m_tiles->GetOffset(2, w2, h2);

			fseek(master, w2h2, SEEK_SET);
			fread(&m_terrainTile2, sizeof(CTerrainTile), 1, master);


			int w3 = w;
			int h3 = h;

			int w3h3 = m_tiles->GetOffset(2, w3, h3);

			fseek(master, w3h3, SEEK_SET);
			fread(&m_terrainTile3, sizeof(CTerrainTile), 1, master);


			int w4 = w;
			int h4 = ((h - 1) < 0) ? 0 : h - 1;

			int w4h4 = m_tiles->GetOffset(2, w4, h4);

			fseek(master, w4h4, SEEK_SET);
			fread(&m_terrainTile4, sizeof(CTerrainTile), 1, master);


			float sumX = m_terrainTile1.m_f.m_n.x + m_terrainTile2.m_c.m_n.x + m_terrainTile2.m_d.m_n.x + m_terrainTile3.m_a.m_n.x + m_terrainTile4.m_e.m_n.x + m_terrainTile4.m_b.m_n.x;
			float sumY = m_terrainTile1.m_f.m_n.y + m_terrainTile2.m_c.m_n.y + m_terrainTile2.m_d.m_n.y + m_terrainTile3.m_a.m_n.y + m_terrainTile4.m_e.m_n.y + m_terrainTile4.m_b.m_n.y;
			float sumZ = m_terrainTile1.m_f.m_n.z + m_terrainTile2.m_c.m_n.z + m_terrainTile2.m_d.m_n.z + m_terrainTile3.m_a.m_n.z + m_terrainTile4.m_e.m_n.z + m_terrainTile4.m_b.m_n.z;

			float aX = sumX / 6.0f;
			float aY = sumY / 6.0f;
			float aZ = sumZ / 6.0f;

			CVec3f nvn = CVec3f(aX, aY, aZ);

			nvn.Normalize();

			m_terrainTile1.m_f.m_n.x = m_terrainTile2.m_c.m_n.x = m_terrainTile2.m_d.m_n.x = m_terrainTile3.m_a.m_n.x = m_terrainTile4.m_e.m_n.x = m_terrainTile4.m_b.m_n.x = nvn.m_p.x;
			m_terrainTile1.m_f.m_n.y = m_terrainTile2.m_c.m_n.y = m_terrainTile2.m_d.m_n.y = m_terrainTile3.m_a.m_n.y = m_terrainTile4.m_e.m_n.y = m_terrainTile4.m_b.m_n.y = nvn.m_p.y;
			m_terrainTile1.m_f.m_n.z = m_terrainTile2.m_c.m_n.z = m_terrainTile2.m_d.m_n.z = m_terrainTile3.m_a.m_n.z = m_terrainTile4.m_e.m_n.z = m_terrainTile4.m_b.m_n.z = nvn.m_p.z;


			fseek(master, w1h1, SEEK_SET);
			fwrite(&m_terrainTile1, sizeof(CTerrainTile), 1, master);

			fseek(master, w2h2, SEEK_SET);
			fwrite(&m_terrainTile2, sizeof(CTerrainTile), 1, master);

			fseek(master, w3h3, SEEK_SET);
			fwrite(&m_terrainTile3, sizeof(CTerrainTile), 1, master);

			fseek(master, w4h4, SEEK_SET);
			fwrite(&m_terrainTile4, sizeof(CTerrainTile), 1, master);
		}
	}

	fclose(master);

	delete m_tiles;




	printf("\nWriting Vertices from tiles\n");

	m_err = fopen_s(&master, "master.txt", "rb");

	if (m_err)
	{
		printf("\nError opening master.txt rb\n");

		return;
	}

	entity->GetKeyValue("compile", &name);

	
	m_terrainFilename = new CString(m_local.m_installPath->m_text);

	m_terrainFilename->Append(m_local.m_assetDirectory->m_text);
	m_terrainFilename->Append(name);

	m_err = fopen_s(&verticesFile, m_terrainFilename->m_text, "wb");

	if (verticesFile == 0)
	{
		return;
	}

	m_vertexCount = 0;

	m_tiles = new CHeapArray(false, sizeof(CTerrainTile), 2, m_terrainWidth - 1, m_terrainHeight - 1);

	for (int h = 0; h < m_terrainHeight - 1; h++)
	{
		for (int w = 0; w < m_terrainWidth - 1; w++)
		{
#ifdef _DEBUG
			printf("H:%03iW:%03i\r", h + 1, w + 1);
#endif

			int wh = m_tiles->GetOffset(2, w, h);

			fseek(master, wh, SEEK_SET);
			fread(&m_terrainTile1, sizeof(CTerrainTile), 1, master);

			fwrite(&m_terrainTile1.m_a, sizeof(CVertexNT), 1, verticesFile);

			m_vertexCount++;
		}

		fwrite(&m_terrainTile1.m_c, sizeof(CVertexNT), 1, verticesFile);

		m_vertexCount++;
	}

	for (int w = 0; w < m_terrainWidth - 1; w++)
	{
		// m_terrainHeight - 2 for last tile
		int wh = m_tiles->GetOffset(2, w, m_terrainHeight - 2);

		fseek(master, wh, SEEK_SET);
		fread(&m_terrainTile1, sizeof(CTerrainTile), 1, master);

		fwrite(&m_terrainTile1.m_b, sizeof(CVertexNT), 1, verticesFile);

		m_vertexCount++;
	}

	fwrite(&m_terrainTile1.m_f, sizeof(CVertexNT), 1, verticesFile);

	m_vertexCount++;


	if (master)
	{
		fclose(master);
	}

	if (verticesFile)
	{
		fclose(verticesFile);
	}

	delete m_tiles;

	delete m_terrainFilename;





	printf("\nWriting Blend Masks\n");

	m_err = fopen_s(&master, "master.txt", "rb");

	if (m_err)
	{
		return;
	}


	entity->GetKeyValue("texture0", &name);

	sscanf_s(name, "%i %i %s", &m_xRepeat, &m_yRepeat, m_textureName, LONG_STRING);


	m_maskFilename = new CString(m_local.m_installPath->m_text);

	m_maskFilename->Append(m_local.m_assetDirectory->m_text);
	m_maskFilename->Append(m_textureName);


	m_err = fopen_s(&maskFile, m_maskFilename->m_text, "wb");

	if (m_err)
	{
		return;
	}

	m_maskImage = new CBmpImage();
	m_maskImage->WriteBitmapHeader(m_terrainWidth, m_terrainHeight, maskFile);
	m_maskImage->WriteColorPalette(maskFile);



	entity->GetKeyValue("texture1", &name);

	sscanf_s(name, "%i %i %s", &m_xRepeat, &m_yRepeat, m_textureName, LONG_STRING);

	m_lightmapFilename = new CString(m_local.m_installPath->m_text);

	m_lightmapFilename->Append(m_local.m_assetDirectory->m_text);
	m_lightmapFilename->Append(m_textureName);

	m_err = fopen_s(&lightmapFile, m_lightmapFilename->m_text, "wb");

	if (m_err)
	{
		return;
	}

	m_lightmapImage = new CBmpImage();
	m_lightmapImage->WriteBitmapHeader(m_terrainWidth, m_terrainHeight, lightmapFile);
	m_lightmapImage->WriteGreyscalePalette(lightmapFile);

	m_tiles = new CHeapArray(false, sizeof(CTerrainTile), 2, m_terrainWidth - 1, m_terrainHeight - 1);

	BYTE p = 0;

	for (int h = 0; h < m_terrainHeight - 1; h++)
	{
		for (int w = 0; w < m_terrainWidth - 1; w++)
		{
#ifdef _DEBUG
			printf("H:%03iW:%03i\r", h + 1, w + 1);
#endif

			int w1h1 = m_tiles->GetOffset(2, w, h);

			fseek(master, w1h1, SEEK_SET);
			fread(&m_terrainTile1, sizeof(CTerrainTile), 1, master);

			CVec3f n = CVec3f(m_terrainTile1.m_a.m_n.x, m_terrainTile1.m_a.m_n.y, m_terrainTile1.m_a.m_n.z);

			n.RadiusNormalize();

			float NdL = n.Dot(&m_down);

			float NdLl = n.Dot(&m_down);

			p = (BYTE)(255.0f * fabsf(NdLl));
			fwrite(&p, sizeof(BYTE), 1, lightmapFile);

			p = 0x00;

			char* surface = {};
			
			entity->GetKeyValue("surface0", &surface);

			if (NdL < -0.85)
			{
				p = 0x00;

				entity->GetKeyValue("surface0", &surface);
			}
			else if (NdL < -0.75)
			{
				p = 0x01;

				entity->GetKeyValue("surface1", &surface);
			}
			else if (NdL < -0.65)
			{
				p = 0x02;

				entity->GetKeyValue("surface2", &surface);
			}
			else if (NdL < -0.45)
			{
				p = 0x03;

				entity->GetKeyValue("surface3", &surface);
			}

			fwrite(&p, sizeof(BYTE), 1, maskFile);










			CVec3f* A = (CVec3f*)m_vertices->GetElement(2, w, h);
			CVec3f* B = (CVec3f*)m_vertices->GetElement(2, w, h + 1);
			CVec3f* C = (CVec3f*)m_vertices->GetElement(2, w + 1, h);

			CVec3f n1 = n1.Normal(B, A, C);

			CVec3f* D = (CVec3f*)m_vertices->GetElement(2, w + 1, h);
			CVec3f* E = (CVec3f*)m_vertices->GetElement(2, w, h + 1);
			CVec3f* F = (CVec3f*)m_vertices->GetElement(2, w + 1, h + 1);

			CVec3f n2 = n2.Normal(D, F, E);

			CCollisionPrimitive cp;
			CShaderMaterial sm;

			sm.SetSurface(surface, 1);

			cp.m_surface = sm.m_surface;

			memcpy(&cp.m_a, A, sizeof(CVec3f));
			memcpy(&cp.m_b, B, sizeof(CVec3f));
			memcpy(&cp.m_c, C, sizeof(CVec3f));
			memcpy(&cp.m_n, &n1, sizeof(CVec3f));

			WriteCollisionPrimitiveToSectors(&cp);

			memcpy(&cp.m_a, D, sizeof(CVec3f));
			memcpy(&cp.m_b, E, sizeof(CVec3f));
			memcpy(&cp.m_c, F, sizeof(CVec3f));
			memcpy(&cp.m_n, &n2, sizeof(CVec3f));

			WriteCollisionPrimitiveToSectors(&cp);











		}

		p = 0;

		fwrite(&p, sizeof(BYTE), 1, maskFile);
		fwrite(&p, sizeof(BYTE), 1, lightmapFile);
	}

	p = 0;

	for (int w = 0; w < m_terrainWidth; w++)
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

	delete m_tiles;
	delete m_vertices;
	delete m_maskImage;
	delete m_maskFilename;
	delete m_lightmapImage;
	delete m_lightmapFilename;


	printf("\nnumber of vertices:%i\n", m_vertexCount);


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
		WriteUVCoord(tu, tv - m_vIncrement);
	}
	else
	{
		WriteUVCoord(0.0f, 0.0f);
	}


	fwrite(C, sizeof(CVec3f), 1, master);
	WriteNormal(n1);

	if (uuv)
	{
		WriteUVCoord(tu + m_uIncrement, tv);
	}
	else
	{
		WriteUVCoord(1.0f, 1.0f);
	}



	fwrite(D, sizeof(CVec3f), 1, master);
	WriteNormal(n2);

	if (uuv)
	{
		WriteUVCoord(tu + m_uIncrement, tv);
	}
	else
	{
		WriteUVCoord(1.0f, 1.0f);
	}


	fwrite(E, sizeof(CVec3f), 1, master);
	WriteNormal(n2);

	if (uuv)
	{
		WriteUVCoord(tu, tv - m_vIncrement);
	}
	else
	{
		WriteUVCoord(0.0f, 0.0f);
	}


	fwrite(F, sizeof(CVec3f), 1, master);
	WriteNormal(n2);

	if (uuv)
	{
		WriteUVCoord(tu + m_uIncrement, tv - m_vIncrement);
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