#ifndef TEST_TEST_H
#define TEST_TEST_H

#include <base/system.h>

#include <cstddef>
#include <memory>

class IStorage;

class CTestInfo
{
public:
	CTestInfo();
	~CTestInfo();
	std::unique_ptr<IStorage> CreateTestStorage();
	const char *StoragePath() const { return m_aStoragePath; }
	bool m_DeleteTestStorageFilesOnSuccess = true;
	void Filename(char *pBuffer, size_t BufferLength, const char *pSuffix);
	char m_aFilenamePrefix[128];
	char m_aFilename[128];
	char m_aStoragePath[IO_MAX_PATH_LENGTH];
	bool m_HasCreatedStoragePath = false;
};
#endif // TEST_TEST_H
