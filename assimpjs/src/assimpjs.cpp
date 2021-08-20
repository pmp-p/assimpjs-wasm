#include "assimpjs.hpp"

#include <assimp/Importer.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stdexcept>
#include <stdio.h>
#include <iostream>

static std::string GetFileName (const std::string& path)
{
	size_t lastSeparator = path.find_last_of ('/');
	if (lastSeparator == std::wstring::npos) {
		return path;
	}
	return path.substr (lastSeparator, path.length () - lastSeparator);
}

FileList::FileList () :
	files ()
{
}

void FileList::AddFile (const std::string& path, const std::vector<std::uint8_t>& content)
{
	files.push_back ({ path, content });
}

const File* FileList::GetFile (size_t index) const
{
	return &files[index];
}

const File* FileList::GetFile (const std::string& path) const
{
	std::string name = GetFileName (path);
	for (const File& file : files) {
		std::string fileName = GetFileName (file.path);
		if (file.path == path) {
			return &file;
		}
	}
	return nullptr;
}

#ifdef EMSCRIPTEN
void FileList::AddFileEmscripten (const std::string& path, const emscripten::val& content)
{
	std::vector<std::uint8_t> contentArr = emscripten::vecFromJSArray<std::uint8_t> (content);
	AddFile (path, contentArr);
}
#endif

int MeaningOfLife ()
{
	return 42;
}

class JSIOStream: public Assimp::IOStream
{
public:
	JSIOStream (const File& file) :
		file (file),
		position (0)
	{
	}

	virtual ~JSIOStream ()
	{
	}
	
	virtual size_t Read (void* pvBuffer, size_t pSize, size_t pCount) override
	{
		size_t memSize = pSize * pCount;
		memcpy (pvBuffer, &file.content[0], memSize);
		position += memSize;
		return memSize;
	}

	virtual size_t Write (const void* pvBuffer, size_t pSize, size_t pCount) override
	{
		throw std::logic_error ("not implemented");
	}
	
	virtual aiReturn Seek (size_t pOffset, aiOrigin pOrigin) override
	{
		switch (pOrigin) {
			case aiOrigin_SET:
				position = pOffset;
				break;
			case aiOrigin_CUR:
				position += pOffset;
				break;
			case aiOrigin_END:
				position = file.content.size () - pOffset;
				break;
		}
		return aiReturn::aiReturn_SUCCESS;
	}

	virtual size_t Tell () const override
	{
		return position;
	}

	virtual size_t FileSize () const override
	{
		return file.content.size ();
	}

	virtual void Flush () override
	{

	}

private:
	const File&		file;
	size_t			position;
};

class JSIOSystem : public Assimp::IOSystem
{
public:
	JSIOSystem (const FileList& fileList) :
		fileList (fileList)
	{
	}

	virtual ~JSIOSystem ()
	{
	
	}

	virtual bool Exists (const char* pFile) const override
	{
		return fileList.GetFile (pFile) != nullptr;
	}

	virtual char getOsSeparator () const override
	{
		return '/';
	}

	virtual Assimp::IOStream* Open (const char* pFile, const char* pMode) override
	{
		const File* foundFile = fileList.GetFile (pFile);
		if (foundFile == nullptr) {
			return nullptr;
		}
		return new JSIOStream (*foundFile);
	}

	virtual void Close (Assimp::IOStream* pFile) override
	{
		delete pFile;
	}

private:
	const FileList& fileList;
};

std::string ImportFile (const FileList& fileList)
{
	Assimp::Importer importer;
	importer.SetIOHandler (new JSIOSystem (fileList));
	const aiScene* scene = importer.ReadFile (fileList.GetFile (0)->path,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType);
	if (scene == nullptr) {
		return "error";
	}
	return "success";
}

#ifdef EMSCRIPTEN

EMSCRIPTEN_BINDINGS (assimpjs)
{
	emscripten::class_<FileList> ("FileList")
		.constructor<> ()
		.function ("AddFile", &FileList::AddFileEmscripten)
	;

	emscripten::function<int> ("MeaningOfLife", &MeaningOfLife);
	emscripten::function<std::string, const FileList&> ("ImportFile", &ImportFile);
}

#endif