#include <stdio.h>
#include <string.h>
#include <SGSLaserRangingModule.h>

typedef int SGSLrmTestResult;

#define SGS_LRM_TEST_SUCCESS    0
#define SGS_LRM_TEST_FAILURE   -1

typedef SGSLrmTestResult (*SGSLrmTestCase)(SGSLrmHandle handle, void* userData);

typedef struct {
    int rangeValue;
} SGSLrmTestSetRangeData;

static SGSLrmTestSetRangeData g_testSetRangeDataSet[] = {
    { 5 },
    { 10 },
    { 30 },
    { 50 },
    { 80 }
};

static SGSLrmTestResult TestSetRange(SGSLrmHandle handle, void* userData);

typedef struct {
    const char* name;
    SGSLrmTestCase test;
    void* userData;
} SGSLrmTestEntry;

static SGSLrmTestEntry g_testCases[] = {
    { "TestSetRange[0]", TestSetRange, &g_testSetRangeDataSet[0]},
    { "TestSetRange[1]", TestSetRange, &g_testSetRangeDataSet[1]},
    { "TestSetRange[2]", TestSetRange, &g_testSetRangeDataSet[2]},
    { "TestSetRange[3]", TestSetRange, &g_testSetRangeDataSet[3]},
    { "TestSetRange[4]", TestSetRange, &g_testSetRangeDataSet[4]},
};

int main(int argc, char* argv[])
{
	int testCount = sizeof(g_testCases) / sizeof(SGSLrmTestEntry);

    if (argc == 1)
    {
        // test all
        for (int i = 0; i < testCount; i++)
        {
            SGSLrmHandle handle = { 0 };
            SGSLrmStatus status = { 0 };
            status = SGSLrm_CreateHandle(&handle);
			status = SGSLrm_Connect(handle, "COM3");
			SGSLrmTestEntry* entry = &g_testCases[i];
            printf("Run test: %s\n", entry->name);
            SGSLrmTestResult result = entry->test(handle, entry->userData);
            printf("Result: %ld\n", result);
            SGSLrm_DestroyHandle(handle);
        }
    }
    else
    {
        // test specific
        const char* specificCase = argv[1];

        for (int i = 0; i < testCount; i++)
        {
            SGSLrmTestEntry* entry = &g_testCases[i];
			int nameLength = strlen(entry->name);
            if (strncmp(entry->name, specificCase, nameLength) == 0)
            {
                SGSLrmHandle handle = { 0 };
                SGSLrmStatus status = { 0 };
                status = SGSLrm_CreateHandle(&handle);
                status = SGSLrm_Connect(handle, "COM3");
                printf("Run test: %s\n", entry->name);
                SGSLrmTestResult result = entry->test(handle, entry->userData);
                printf("Result: %ld\n", result);
                SGSLrm_DestroyHandle(handle);

                return 0;
            }
        }
    }

    return 0;
}

static SGSLrmTestResult TestSetRange(SGSLrmHandle handle, void* userData)
{
	SGSLrmTestSetRangeData* data = (SGSLrmTestSetRangeData*)userData;
    
	SGSLrmStatus status = SGSLrm_SetRange(handle, data->rangeValue);
    
    return status == SGS_LRM_SUCCESS ? SGS_LRM_TEST_SUCCESS : SGS_LRM_TEST_FAILURE;
}