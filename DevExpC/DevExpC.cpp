// DevExpC.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "DeviceManager.h"
#define INITGUID
#include <devpkey.h>

#pragma comment(lib, "cfgmgr32")

std::wstring GuidToString(GUID const& guid) {
	WCHAR name[64];
	return SUCCEEDED(::StringFromGUID2(guid, name, _countof(name))) ? name : L"";
}

void DumpDeviceInterfaces() {
	for (auto& di : DeviceManager::EnumDeviceInterfaces()) {
		printf("%ws %ws (%ws) (%ws)\n", GuidToString(di.Guid).c_str(),
			di.FriendlyName.c_str(), di.DeviceFriendlyName.c_str(), di.DeviceDescription.c_str());
	}
}

void DumpDeviceClasses() {
	GUID guid;
	for (auto& name : DeviceManager::EnumDeviceClasses()) {
		if (FAILED(::CLSIDFromString(name.c_str(), &guid)))
			continue;
		auto props = DeviceManager::GetClassPropertyKeys(guid);
		for (auto& prop : props) {
		}
	}
}

void DumpHardwareProfiles() {
	for (auto& profile : DeviceManager::EnumHardwareProfiles()) {
		printf("Profile %u: %ws\n", profile.Index, profile.FriendlyName.c_str());
	}
}

void TestEnum() {
	GUID guid;
	printf("Classes\n");
	for (DWORD i = 0;; i++) {
		if (CR_NO_SUCH_VALUE == ::CM_Enumerate_Classes(i, &guid, CM_ENUMERATE_CLASSES_INSTALLER))
			break;
		printf("%ws\n", GuidToString(guid).c_str());
	}

	printf("Interfaces\n");
	for (DWORD i = 0;; i++) {
		if (CR_NO_SUCH_VALUE == ::CM_Enumerate_Classes(i, &guid, CM_ENUMERATE_CLASSES_INTERFACE))
			break;
		printf("%ws\n", GuidToString(guid).c_str());
	}
	printf("Enumerators\n");
	WCHAR name[MAX_DEVICE_ID_LEN + 1];
	for (DWORD i = 0;; i++) {
		DWORD size = _countof(name);
		if (CR_NO_SUCH_VALUE == ::CM_Enumerate_Enumerators(i, name, &size, 0))
			break;
		printf("%ws\n", name);
		
		::CM_Get_Device_ID_List_Size(&size, name, CM_GETIDLIST_FILTER_ENUMERATOR | CM_GETIDLIST_FILTER_PRESENT);
		auto buffer = std::make_unique<WCHAR[]>(size);
		::CM_Get_Device_ID_List(name, buffer.get(), size, CM_GETIDLIST_FILTER_ENUMERATOR | CM_GETIDLIST_FILTER_PRESENT);
		auto p = buffer.get();
		while (*p) {
			DEVINST inst = 0;
			auto error = ::CM_Locate_DevInst(&inst, p, 0);
			printf("\t%ws (%d)\n", p, error == CR_SUCCESS ? inst : -1);
			p += wcslen(p) + 1;
		}
	}
}

void DisplayProperties(DEVINST inst) {
	auto buffer = std::make_unique<BYTE[]>(2048);
	DEVPROPKEY keys[80];
	ULONG count = _countof(keys);
	::CM_Get_DevNode_Property_Keys(inst, keys, &count, 0);
	DEVPROPTYPE type;
	for (DWORD i = 0; i < count; i++) {
		ULONG size = 2048;
		::CM_Get_DevNode_Property(inst, keys + i, &type, buffer.get(), &size, 0);
		if (type == DEVPROP_TYPE_STRING && keys[i] == DEVPKEY_NAME) {
			printf("\t(%ws)\n", (PCWSTR)buffer.get());
		}
		else if (type == DEVPROP_TYPE_STRING_INDIRECT) {
			int zz = 9;
		}
	}
}

void EnumSiblingDevNodes(DEVINST inst, int indent = 0);

void EnumChildDevNodes(DEVINST inst, int indent = 1) {
	WCHAR name[256];
	DEVINST child = 0;
	::CM_Get_Child(&child, inst, 0);
	while (child) {
		::CM_Get_Device_ID(child, name, _countof(name), 0);
		printf("%s%ws\n", std::string(indent, ' ').c_str(), name);
		DisplayProperties(child);
		EnumSiblingDevNodes(child, indent);
		EnumChildDevNodes(child, indent + 1);
		DEVINST temp = 0;
		::CM_Get_Child(&temp, child, 0);
		child = temp;
	}
}

void EnumSiblingDevNodes(DEVINST inst, int indent) {
	WCHAR name[256];
	DEVINST child = 0;
	::CM_Get_Sibling(&child, inst, 0);
	while (child) {
		::CM_Get_Device_ID(child, name, _countof(name), 0);
		printf("%s%ws\n", std::string(indent, ' ').c_str(), name);
		DisplayProperties(child);
		DEVINST sib = 0;
		::CM_Get_Sibling(&sib, child, 0);
		EnumChildDevNodes(child, indent + 1);
		child = sib;
		//EnumSiblingDevNodes(child, indent);
	}
}

void EnumDevNodes() {
	//::CM_Locate_DevInst(
	DEVINST inst;
	WCHAR name[256];
	DEVPROPKEY keys[30];
	ULONG count = _countof(keys);
	auto buffer = std::make_unique<BYTE[]>(2048);

	if (CR_SUCCESS == ::CM_Locate_DevNode(&inst, nullptr, CM_LOCATE_DEVNODE_NORMAL)) {
		::CM_Get_Device_ID(inst, name, _countof(name), 0);
		printf("%ws\n", name);
		DisplayProperties(inst);
		::CM_Get_DevNode_Property_Keys(inst, keys, &count, 0);
		DEVPROPTYPE type;
		for (DWORD i = 0; i < count; i++) {
			ULONG size = 2048;
			::CM_Get_DevNode_Property(inst, keys + i, &type, buffer.get(), &size, 0);
			//printf("Type: %u\n", type);
		}

		EnumChildDevNodes(inst);
		EnumSiblingDevNodes(inst);


	}
}

int main() {
	//DumpDeviceInterfaces();
	//DumpHardwareProfiles();
	//DumpDeviceClasses();
	EnumDevNodes();
	//TestEnum();

	return 0;
}
