#include "../GameCommon/framework.h"

#include "../GameCommon/CBmpImage.h"
#include "../GameCommon/CBrush.h"
#include "../GameCommon/CCollision.h"
#include "../GameCommon/CCollisionPrimitive.h"
#include "../GameCommon/CDirectoryList.h"
#include "../GameCommon/CEntity.h"
#include "../GameCommon/CHeap.h"
#include "../GameCommon/CHeapArray.h"
#include "../GameCommon/CList.h"
#include "../GameCommon/CLocal.h"
#include "../GameCommon/CScript.h"
#include "../GameCommon/CSector.h"
#include "../GameCommon/CShaderMaterial.h"
#include "../GameCommon/CTerrainTile.h"
#include "../GameCommon/CTgaImage.h"
#include "../GameCommon/CVec3f.h"
#include "../GameCommon/CVertexNT.h"

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

CHeap* m_heapCollision;
CHeap* m_heapMap;
CListNode* m_node;
CLocal m_local;
CScript m_mapScript;
CScript m_objectScript;
CScript m_shaderScript;
CSector* m_sector;
CShaderMaterial m_masterMaterials[MAX_MATERIALS];
CShaderMaterial m_materials[MAX_MATERIALS];
CTgaImage m_images[MAX_MATERIALS];
CVec3f m_dir;
CVec3f m_sectorNormal[12];
CVec3f m_sectorTriangle[12][3];
CVec3i m_mapSize;

errno_t m_err;

FILE* m_fCollision;
FILE* m_fMap;

int32_t m_brushCount = 0;
int32_t m_count = 0;
int32_t m_entityCollisionCount = 0;
int32_t m_entityCount = 0;
int32_t m_entityMapCount = 0;
int32_t m_masterMaterialsCount = 0;
int32_t m_materialsCount = 0;
int32_t m_sectorSize;

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

float m_u;
float m_uIncrement;
float m_v;
float m_terrainHeight;
float m_vIncrement;
float m_xIncrement;
float m_zIncrement;

int32_t m_primSize;
int32_t m_terrainDepth;
int32_t m_terrainWidth;
int32_t m_vertexCount;
int32_t m_xRepeat;
int32_t m_yRepeat;

void LoadAllShaders();
void ParseShaderScript();
void WriteMaterials();

void WriteBrush(CBrush* brush);
void WriteCollisionPrimitiveToSectors(CCollisionPrimitive* collisionPrimitive);

void CreateTerrain(CEntity* entity);
void WriteTile(CVec3f* A, CVec3f* B, CVec3f* C, CVec3f* n1, CVec3f* D, CVec3f* E, CVec3f* F, CVec3f* n2, bool uuv, float tu, float tv);
void WriteNormal(CVec3f* n);
void WriteUVCoord(float tu, float tv);
/*
*/
int32_t main(int32_t argc, char* argv[])
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

	int32_t slash = 0;

	for (int32_t i = 0; i < strlen(m_mapName); i++)
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
			case CEntity::Type::PROJECTOR:
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

	fwrite(&m_entityMapCount, sizeof(int32_t), 1, m_fMap);
	fwrite(&m_entityCollisionCount, sizeof(int32_t), 1, m_fCollision);

	for (int32_t i = 0; i < m_entityCount; i++)
	{
		switch (m_entity[i].m_type)
		{
		case CEntity::Type::WORLDSPAWN:
		{
			m_entity[i].WriteKeyValues(m_fMap);
			m_entity[i].WriteKeyValues(m_fCollision);

			m_node = m_entity[i].m_brushes->m_list;

			while ((m_node) && (m_node->m_object))
			{
				CBrush* brush = (CBrush*)m_node->m_object;

				WriteBrush(brush);

				m_node = m_node->m_next;
			}

			fwrite(&MINUS_ONE, sizeof(int32_t), 1, m_fMap);
			fwrite(&MINUS_ONE, sizeof(int32_t), 1, m_fCollision);

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
		case CEntity::Type::PROJECTOR:
		{
			m_entity[i].WriteKeyValues(m_fMap);

			break;
		}
		case CEntity::Type::TERRAIN:
		{
			m_entity[i].WriteKeyValues(m_fMap);
			m_entity[i].WriteKeyValues(m_fCollision);

			CreateTerrain(&m_entity[i]);

			fwrite(&MINUS_ONE, sizeof(int32_t), 1, m_fCollision);

			break;
		}
		default:
		{
			for (int32_t e = 0; e < m_entity[i].m_keyValueCount; e++)
			{
				printf("Unkown entity:%s %s\n", m_entity[i].m_keyValue[e].m_key, m_entity[i].m_keyValue[e].m_value);
			}

			break;
		}
		}
	}

	fclose(m_fMap);
	fclose(m_fCollision);

	SAFE_DELETE(m_sector);

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

	m_node = m_directoryList->m_paths->m_list;

	while ((m_node) && (m_node->m_object))
	{
		CString* shader = (CString*)m_node->m_object;

		m_shaderScript.InitBuffer(shader->m_text);

		ParseShaderScript();

		m_node = m_node->m_next;
	}

	SAFE_DELETE(m_directoryList);
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
	fwrite(&m_materialsCount, sizeof(int32_t), 1, m_fMap);

#ifdef _DEBUG
	printf("materials:%i\n\n", m_materialsCount);
#endif

	for (int32_t m = 0; m < m_materialsCount; m++)
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
	m_heapMap = new CHeap(32768);

	m_heapCollision = new CHeap(32768);

	for (int32_t s = 0; s < brush->m_sideCount; s++)
	{
		if (strstr(brush->m_brushSide[s].m_shaderMaterial->m_name, "caulk") != 0)
		{
			continue;
		}

		if (brush->m_brushSide[s].m_vertexCount < 3)
		{
			continue;
		}

		m_count = 0;

		for (int32_t v = 0; v < brush->m_brushSide[s].m_vertexCount; v++)
		{
			m_count++;
			v++;
			m_count++;
			v++;
			m_count++;

			if (v == brush->m_brushSide[s].m_vertexCount - 1)
			{
				break;
			}

			v--;
			v--;
		}

		m_heapMap->Append(brush->m_brushSide[s].m_shaderMaterial->m_number);

		m_heapMap->Append(m_count);

		for (int32_t v = 0; v < brush->m_brushSide[s].m_vertexCount; v++)
		{
			m_heapCollision->Append(brush->m_brushSide[s].m_shaderMaterial->m_surface);

			m_heapCollision->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[0].m_v.m_p, sizeof(CVec3f));

			m_heapMap->Append(0);

			v++;

			m_heapCollision->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v].m_v.m_p, sizeof(CVec3f));

			m_heapMap->Append(v);

			v++;

			m_heapCollision->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v].m_v.m_p, sizeof(CVec3f));

			m_heapCollision->Append((unsigned char*)&brush->m_brushSide[s].m_center.m_v.m_n, sizeof(CVec3f));

			m_heapMap->Append(v);

			if (v == brush->m_brushSide[s].m_vertexCount - 1)
			{
				break;
			}

			v--;
			v--;
		}

		m_heapMap->Append(brush->m_brushSide[s].m_vertexCount);

		for (int32_t v = 0; v < brush->m_brushSide[s].m_vertexCount; v++)
		{
			m_heapMap->Append((unsigned char*)&brush->m_brushSide[s].m_vertex[v].m_v, sizeof(CVertexNT));
		}

		fwrite(m_heapMap->m_heap, m_heapMap->m_length, 1, m_fMap);

		m_heapMap->Reset();

		CCollisionPrimitive* collisionPrimitive = (CCollisionPrimitive*)m_heapCollision->m_heap;

		int32_t count = m_heapCollision->m_length / sizeof(CCollisionPrimitive);

		for (int32_t ii = 0; ii < count; ii++)
		{
			WriteCollisionPrimitiveToSectors(&collisionPrimitive[ii]);
		}

		m_heapCollision->Reset();
	}

	SAFE_DELETE(m_heapMap);
	SAFE_DELETE(m_heapCollision);
}

/*
*/
void WriteCollisionPrimitiveToSectors(CCollisionPrimitive* collisionPrimitive)
{
	m_sector->Reset();

	for (int32_t y = 0; y < m_sector->m_gridHeight; y++)
	{
		for (int32_t z = 0; z < m_sector->m_gridDepth; z++)
		{
			for (int32_t x = 0; x < m_sector->m_gridWidth; x++)
			{
				if (m_sector->CheckTriangleOutside(collisionPrimitive))
				{
				}
				else if (m_sector->CheckPointsInside(x, y, z, collisionPrimitive))
				{
					fwrite(&x, sizeof(int32_t), 1, m_fCollision);
					fwrite(&y, sizeof(int32_t), 1, m_fCollision);
					fwrite(&z, sizeof(int32_t), 1, m_fCollision);

					collisionPrimitive->WritePrimitive(m_fCollision);
				}
				else if (m_sector->CheckSectorRayTrace(collisionPrimitive))
				{
					fwrite(&x, sizeof(int32_t), 1, m_fCollision);
					fwrite(&y, sizeof(int32_t), 1, m_fCollision);
					fwrite(&z, sizeof(int32_t), 1, m_fCollision);

					collisionPrimitive->WritePrimitive(m_fCollision);
				}
				else if (m_sector->CheckTriangleRayTrace(collisionPrimitive))
				{
					fwrite(&x, sizeof(int32_t), 1, m_fCollision);
					fwrite(&y, sizeof(int32_t), 1, m_fCollision);
					fwrite(&z, sizeof(int32_t), 1, m_fCollision);

					collisionPrimitive->WritePrimitive(m_fCollision);
				}

				m_sector->NextWidth();
			}

			m_sector->ResetWidth();

			m_sector->NextDepth();
		}

		m_sector->ResetWidth();

		m_sector->ResetDepth();

		m_sector->NextHeight();
	}
}

/*
*/
void CreateTerrain(CEntity* entity)
{
	printf("\nCreating Terrain\n");

	char* name = {};

	entity->GetKeyValue("map", &name);
	entity->GetKeyValue("tileSize", &m_primSize);
	entity->GetKeyValue("height", &m_terrainHeight);
	entity->GetKeyValue("width", &m_terrainWidth);
	entity->GetKeyValue("depth", &m_terrainDepth);
	entity->GetKeyValue("origin", &m_origin);

	float t = m_origin.m_p.z;
	m_origin.m_p.z = m_origin.m_p.y;
	m_origin.m_p.y = t;

	m_heightmapFilename = new CString(m_local.m_installPath->m_text);

	m_heightmapFilename->Append("main/");
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
		m_terrainDepth = m_heightmapImage->m_bmapInfo.bmiHeader.biHeight;

		m_uIncrement = 1.0f / m_terrainWidth;
		m_vIncrement = 1.0f / m_terrainDepth;

		m_u = 0.0f;
		m_v = 1.0f;

		m_pixelStart = (m_heightmapImage->m_pixels8 + (size_t)m_terrainWidth * m_terrainDepth * m_heightmapImage->m_bytesPerPixel) - m_terrainWidth;

		m_xIncrement = (float)(m_terrainWidth / 2) * -m_primSize;
		m_zIncrement = (float)(m_terrainDepth / 2) * -m_primSize;

		m_vertices = new CHeapArray(true, sizeof(CVec3f), 2, m_terrainWidth, m_terrainDepth);

		for (int32_t z = 0; z < m_terrainDepth; z++)
		{
			m_xIncrement = (float)(m_terrainWidth / 2) * -m_primSize;

			for (int32_t x = 0; x < m_terrainWidth; x++)
			{
#ifdef _DEBUG
				printf("D:%03iW:%03i\r", z, x);
#endif
				CVec3f* vertex = (CVec3f*)m_vertices->GetElement(2, x, z);

				vertex->m_p.x = m_xIncrement;
				vertex->m_p.y = (float)(m_heightmapImage->m_paletteEntries[*m_pixelStart].peRed * m_terrainHeight);
				vertex->m_p.z = m_zIncrement;

				vertex->m_p.x += m_origin.m_p.x;
				vertex->m_p.y += m_origin.m_p.y;
				vertex->m_p.z += m_origin.m_p.z;

				m_xIncrement += m_primSize;

				m_pixelStart++;
			}

			m_zIncrement += m_primSize;

			m_pixelStart -= (m_terrainWidth * 2);
		}
	}
	else
	{
		m_uIncrement = 1.0f / m_terrainWidth;
		m_vIncrement = 1.0f / m_terrainDepth;

		m_u = 0.0f;
		m_v = 1.0f;

		m_objectScript.InitBuffer(m_heightmapFilename->m_text);

		if (m_objectScript.CheckEndOfBuffer())
		{
			printf("\nTerrain not found:%s\n", m_heightmapFilename->m_text);

			return;
		}

		m_vertices = new CHeapArray(true, sizeof(CVec3f), 2, m_terrainWidth, m_terrainDepth);

		for (int32_t z = 0; z < m_terrainDepth; z++)
		{
			for (int32_t x = 0; x < m_terrainWidth; x++)
			{
#ifdef _DEBUG
				printf("D:%03iW:%03i\r", z, x);
#endif
				CVec3f* vertex = (CVec3f*)m_vertices->GetElement(2, x, z);

				if (m_objectScript.CheckEndOfBuffer())
				{
					break;
				}

				m_objectScript.MoveToToken("v");

				m_objectScript.Move(2);

				vertex->m_p.x = strtof(m_objectScript.GetToken(), nullptr);
				vertex->m_p.z = strtof(m_objectScript.GetToken(), nullptr);
				vertex->m_p.y = strtof(m_objectScript.GetToken(), nullptr);

				vertex->m_p.x += m_origin.m_p.x;
				vertex->m_p.y += m_origin.m_p.y;
				vertex->m_p.z += m_origin.m_p.z;

				m_objectScript.SkipEndOfLine();
			}
		}
	}

	SAFE_DELETE(m_heightmapFilename);

	/*
	Ground Collision

	B  E-F
	|\  \|
	A-C  D

	BAC is N1
	DFE is N2
	*/
	m_err = fopen_s(&master, "master.txt", "wb");

	if (m_err)
	{
		printf("Error opening master.txt wb\n");

		SAFE_DELETE(m_vertices);
		SAFE_DELETE(m_heightmapImage);

		return;
	}

	printf("\nWriting Master\n");

	for (int32_t z = 0; z < m_terrainDepth - 1; z++)
	{
		for (int32_t x = 0; x < m_terrainWidth - 1; x++)
		{
#ifdef _DEBUG
			printf("D:%03iW:%03i\r", z, x);
#endif
			CVec3f* A = (CVec3f*)m_vertices->GetElement(2, x, z);
			CVec3f* B = (CVec3f*)m_vertices->GetElement(2, x, z + 1);
			CVec3f* C = (CVec3f*)m_vertices->GetElement(2, x + 1, z);

			CVec3f n1 = n1.Normal(B, A, C);

			CVec3f* D = (CVec3f*)m_vertices->GetElement(2, x + 1, z);
			CVec3f* E = (CVec3f*)m_vertices->GetElement(2, x, z + 1);
			CVec3f* F = (CVec3f*)m_vertices->GetElement(2, x + 1, z + 1);

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

	m_tiles = new CHeapArray(false, sizeof(CTerrainTile), 2, m_terrainWidth - 1, m_terrainDepth - 1);

	for (int32_t z = 0; z < m_terrainDepth - 1; z++)
	{
		for (int32_t x = 0; x < m_terrainWidth - 1; x++)
		{
#ifdef _DEBUG
			printf("D:%03iW:%03i\r", z, x);
#endif
			int32_t x1 = ((x - 1) < 0) ? 0 : x - 1;
			int32_t z1 = ((z - 1) < 0) ? 0 : z - 1;

			int32_t x1z1 = m_tiles->GetOffset(2, x1, z1);

			fseek(master, x1z1, SEEK_SET);
			fread(&m_terrainTile1, sizeof(CTerrainTile), 1, master);

			int32_t x2 = ((x - 1) < 0) ? 0 : x - 1;
			int32_t z2 = z;

			int32_t x2z2 = m_tiles->GetOffset(2, x2, z2);

			fseek(master, x2z2, SEEK_SET);
			fread(&m_terrainTile2, sizeof(CTerrainTile), 1, master);

			int32_t x3 = x;
			int32_t z3 = z;

			int32_t x3z3 = m_tiles->GetOffset(2, x3, z3);

			fseek(master, x3z3, SEEK_SET);
			fread(&m_terrainTile3, sizeof(CTerrainTile), 1, master);

			int32_t x4 = x;
			int32_t z4 = ((z - 1) < 0) ? 0 : z - 1;

			int32_t x4z4 = m_tiles->GetOffset(2, x4, z4);

			fseek(master, x4z4, SEEK_SET);
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

			fseek(master, x1z1, SEEK_SET);
			fwrite(&m_terrainTile1, sizeof(CTerrainTile), 1, master);

			fseek(master, x2z2, SEEK_SET);
			fwrite(&m_terrainTile2, sizeof(CTerrainTile), 1, master);

			fseek(master, x3z3, SEEK_SET);
			fwrite(&m_terrainTile3, sizeof(CTerrainTile), 1, master);

			fseek(master, x4z4, SEEK_SET);
			fwrite(&m_terrainTile4, sizeof(CTerrainTile), 1, master);
		}
	}

	fclose(master);

	SAFE_DELETE(m_tiles);


	printf("\nWriting Vertices from tiles\n");

	m_err = fopen_s(&master, "master.txt", "rb");

	if (m_err)
	{
		printf("\nError opening master.txt rb\n");

		return;
	}

	entity->GetKeyValue("compile", &name);

	m_terrainFilename = new CString(m_local.m_installPath->m_text);

	m_terrainFilename->Append("main/");
	m_terrainFilename->Append(name);

	m_err = fopen_s(&verticesFile, m_terrainFilename->m_text, "wb");

	if (verticesFile == 0)
	{
		return;
	}

	m_vertexCount = 0;

	m_tiles = new CHeapArray(false, sizeof(CTerrainTile), 2, m_terrainWidth - 1, m_terrainDepth - 1);

	for (int32_t z = 0; z < m_terrainDepth - 1; z++)
	{
		for (int32_t x = 0; x < m_terrainWidth - 1; x++)
		{
#ifdef _DEBUG
			printf("D:%03iW:%03i\r", z, x);
#endif
			int32_t xz = m_tiles->GetOffset(2, x, z);

			fseek(master, xz, SEEK_SET);

			fread(&m_terrainTile1, sizeof(CTerrainTile), 1, master);

			fwrite(&m_terrainTile1.m_a, sizeof(CVertexNT), 1, verticesFile);

			m_vertexCount++;
		}

		fwrite(&m_terrainTile1.m_c, sizeof(CVertexNT), 1, verticesFile);

		m_vertexCount++;
	}

	for (int32_t x = 0; x < m_terrainWidth - 1; x++)
	{
		// m_terrainDepth - 2 for last tile vertex at max depth
		int32_t xz = m_tiles->GetOffset(2, x, m_terrainDepth - 2);

		fseek(master, xz, SEEK_SET);
		
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

	SAFE_DELETE(m_tiles);
	SAFE_DELETE(m_terrainFilename);


	printf("\nWriting Blend Masks\n");

	m_err = fopen_s(&master, "master.txt", "rb");

	if (m_err)
	{
		return;
	}

	entity->GetKeyValue("texture0", &name);

	sscanf_s(name, "%i %i %s", &m_xRepeat, &m_yRepeat, m_textureName, LONG_STRING);

	m_maskFilename = new CString(m_local.m_installPath->m_text);

	m_maskFilename->Append("main/");
	m_maskFilename->Append(m_textureName);

	m_err = fopen_s(&maskFile, m_maskFilename->m_text, "wb");

	if (m_err)
	{
		return;
	}

	m_maskImage = new CBmpImage();
	
	m_maskImage->WriteBitmapHeader(m_terrainWidth, m_terrainDepth, maskFile);
	m_maskImage->WriteColorPalette(maskFile);

	entity->GetKeyValue("texture1", &name);

	sscanf_s(name, "%i %i %s", &m_xRepeat, &m_yRepeat, m_textureName, LONG_STRING);

	m_lightmapFilename = new CString(m_local.m_installPath->m_text);

	m_lightmapFilename->Append("main/");
	m_lightmapFilename->Append(m_textureName);

	m_err = fopen_s(&lightmapFile, m_lightmapFilename->m_text, "wb");

	if (m_err)
	{
		return;
	}

	m_lightmapImage = new CBmpImage();

	m_lightmapImage->WriteBitmapHeader(m_terrainWidth, m_terrainDepth, lightmapFile);
	m_lightmapImage->WriteGreyscalePalette(lightmapFile);

	m_tiles = new CHeapArray(false, sizeof(CTerrainTile), 2, m_terrainWidth - 1, m_terrainDepth - 1);

	BYTE p = 0;

	for (int32_t z = 0; z < m_terrainDepth - 1; z++)
	{
		for (int32_t x = 0; x < m_terrainWidth - 1; x++)
		{
#ifdef _DEBUG
			printf("D:%03iW:%03i\r", z, x);
#endif
			int32_t x1z1 = m_tiles->GetOffset(2, x, z);

			fseek(master, x1z1, SEEK_SET);
			fread(&m_terrainTile1, sizeof(CTerrainTile), 1, master);

			CVec3f n = CVec3f(m_terrainTile1.m_a.m_n.x, m_terrainTile1.m_a.m_n.y, m_terrainTile1.m_a.m_n.z);

			n.RadiusNormalize();

			float NdL = n.Dot(&m_down);

			p = (BYTE)(255.0f * fabsf(NdL));
			
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

			CVec3f* A = (CVec3f*)m_vertices->GetElement(2, x, z);
			CVec3f* B = (CVec3f*)m_vertices->GetElement(2, x, z + 1);
			CVec3f* C = (CVec3f*)m_vertices->GetElement(2, x + 1, z);

			CVec3f n1 = n1.Normal(B, A, C);

			CVec3f* D = (CVec3f*)m_vertices->GetElement(2, x + 1, z);
			CVec3f* E = (CVec3f*)m_vertices->GetElement(2, x, z + 1);
			CVec3f* F = (CVec3f*)m_vertices->GetElement(2, x + 1, z + 1);

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

	for (int32_t x = 0; x < m_terrainWidth; x++)
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

	SAFE_DELETE(m_tiles);
	SAFE_DELETE(m_vertices);
	SAFE_DELETE(m_maskImage);
	SAFE_DELETE(m_maskFilename);
	SAFE_DELETE(m_lightmapImage);
	SAFE_DELETE(m_lightmapFilename);

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