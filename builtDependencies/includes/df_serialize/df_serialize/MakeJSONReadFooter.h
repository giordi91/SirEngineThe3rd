inline bool LoadTextFile(const char* fileName, TDYNAMICARRAY<char>& data)
{
    // open the file if we can
    FILE* file = nullptr;
    fopen_s(&file, fileName, "rb");
    if (!file)
        return false;

    // get the file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // read the file into memory and return success.
    // don't forget the null terminator
    TDYNAMICARRAY_RESIZE(data, size + 1);
    fread(&data[0], 1, size, file);
    fclose(file);
    data[TDYNAMICARRAY_SIZE(data) - 1] = 0;
    return true;
}

// Read a structure from a JSON string
template<typename TROOT>
bool ReadFromJSONBuffer(TROOT& root, const char* data)
{
    rapidjson::Document document;
    rapidjson::ParseResult ok = document.Parse<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag>(data);
    if (!ok)
    {
        size_t errorOffset = ok.Offset();

        // count what line we are on
        int lineNumber = 1;
        {
            size_t index = errorOffset;
            while (index > 0)
            {
                if (data[index] == '\n')
                    lineNumber++;
                index--;
            }
        }

        // back up to the beginning of the line
        while (errorOffset > 0 && data[errorOffset] != '\n')
            errorOffset--;
        if (errorOffset > 0)
            errorOffset++;

        // get the next couple lines from the error
        size_t end = errorOffset;
        for (int i = 0; i < 4; ++i)
        {
            while (data[end] != 0 && data[end] != '\n')
                end++;
            if (data[end] != 0)
                end++;
        }

        TSTRING s;
        TSTRING_RESIZE(s, end - errorOffset + 1);
        memcpy(&s[0], &data[errorOffset], end - errorOffset);
        s[end - errorOffset] = 0;

        DFS_LOG("JSON parse error line %i\n%s\n%s\n", lineNumber, GetParseError_En(ok.Code()), &s[0]);
        return false;
    }

    return JSONRead(root, document);
}

template<typename TROOT>
bool ReadFromJSONBuffer(TROOT& root, const TDYNAMICARRAY<char>& data)
{
    return ReadFromJSONBuffer(root, &data[0]);
}

template<typename TROOT>
bool ReadFromJSONBuffer(TROOT& root, const TSTRING& data)
{
    return ReadFromJSONBuffer(root, &data[0]);
}

// Read a structure from a JSON file
template<typename TROOT>
bool ReadFromJSONFile(TROOT& root, const char* fileName)
{
    TDYNAMICARRAY<char> fileData;
    if (!LoadTextFile(fileName, fileData))
    {
        DFS_LOG("Could not read file %s", fileName);
        return false;
    }

    return ReadFromJSONBuffer(root, fileData);
}
