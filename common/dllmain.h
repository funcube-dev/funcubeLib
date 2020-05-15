// dllmain.h : Declaration of module class.

class CFuncubeLibModule : public CAtlDllModuleT< CFuncubeLibModule >
{
public :
	DECLARE_LIBID(LIBID_FuncubeLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_FuncubeLib, "{0C5BE80D-F4B8-430A-B97E-EAAE71BFD7F7}")
};

extern class CFuncubeLibModule _AtlModule;
