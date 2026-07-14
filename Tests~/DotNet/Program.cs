using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential)]
internal struct Capabilities
{
    public uint abiVersion;
    public uint apiVersion;
    public uint vendorId;
    public uint deviceId;
    public uint pluginLoaded;
    public uint vulkanDeviceReady;
    public uint dispatchLoaded;
    public uint accelerationStructure;
    public uint rayTracingPipeline;
    public uint rayQuery;
    public uint bufferDeviceAddress;
    public uint maxRecursionDepth;
    public uint shaderGroupHandleSize;
    public uint shaderGroupBaseAlignment;
}

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
internal delegate uint GetAbiVersionDelegate();

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
internal delegate int GetCapabilitiesDelegate(IntPtr capabilities);

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
internal delegate int IntFunctionDelegate();

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
internal delegate int SetShaderDirectoryDelegate(IntPtr path);

internal static class Program
{
    private static readonly string[] RequiredExports =
    {
        "CLOISimRt_GetAbiVersion",
        "CLOISimRt_GetCapabilities",
        "CLOISimRt_IsNativeBackendAvailable",
        "CLOISimRt_SetShaderDirectory",
        "CLOISimRt_InitializeDepthPipeline",
        "CLOISimRt_IsDepthPipelineReady",
        "CLOISimRt_GetRenderEventFunc",
        "UnityPluginLoad",
        "UnityPluginUnload",
    };

    private static void Require(bool condition, string message)
    {
        if (!condition)
            throw new InvalidOperationException(message);
    }

    private static T Load<T>(IntPtr library, string name)
        where T : Delegate
    {
        var address = NativeLibrary.GetExport(library, name);
        Require(address != IntPtr.Zero, $"zero export address: {name}");
        return Marshal.GetDelegateForFunctionPointer<T>(address);
    }

    public static int Main()
    {
        var path = Environment.GetEnvironmentVariable(
            "CLOISIM_RT_PLUGIN_PATH");
        Require(!string.IsNullOrWhiteSpace(path),
            "CLOISIM_RT_PLUGIN_PATH is required");
        Require(File.Exists(path), $"plugin not found: {path}");

        Require(Marshal.SizeOf<Capabilities>() == 56,
            "Capabilities size must be 56 bytes");
        Require(Marshal.OffsetOf<Capabilities>(
            nameof(Capabilities.dispatchLoaded)).ToInt32() == 24,
            "dispatchLoaded offset must be 24");
        Require(Marshal.OffsetOf<Capabilities>(
            nameof(Capabilities.shaderGroupBaseAlignment)).ToInt32() == 52,
            "shaderGroupBaseAlignment offset must be 52");

        var library = NativeLibrary.Load(path!);
        try
        {
            foreach (var export in RequiredExports)
            {
                Require(NativeLibrary.TryGetExport(
                    library, export, out var address),
                    $"missing export: {export}");
                Require(address != IntPtr.Zero,
                    $"zero export address: {export}");
            }

            var getAbi = Load<GetAbiVersionDelegate>(
                library, "CLOISimRt_GetAbiVersion");
            Require(getAbi() == 2U, "ABI version must be 2");

            var getCapabilities = Load<GetCapabilitiesDelegate>(
                library, "CLOISimRt_GetCapabilities");
            var isNative = Load<IntFunctionDelegate>(
                library, "CLOISimRt_IsNativeBackendAvailable");
            var initializeDepth = Load<IntFunctionDelegate>(
                library, "CLOISimRt_InitializeDepthPipeline");
            var isDepthReady = Load<IntFunctionDelegate>(
                library, "CLOISimRt_IsDepthPipelineReady");
            var setShaderDirectory = Load<SetShaderDirectoryDelegate>(
                library, "CLOISimRt_SetShaderDirectory");

            Require(getCapabilities(IntPtr.Zero) == -1,
                "GetCapabilities(null) must fail safely");
            Require(isNative() == 0,
                "native backend must be unavailable before Unity init");
            Require(initializeDepth() == -1,
                "depth pipeline init must fail before device init");
            Require(isDepthReady() == 0,
                "depth pipeline must not be ready before device init");
            Require(setShaderDirectory(IntPtr.Zero) == -1,
                "null shader directory must fail safely");

            Console.WriteLine(
                "[OK] exports, ABI layout, delegates, and failure contracts passed");
            return 0;
        }
        finally
        {
            NativeLibrary.Free(library);
        }
    }
}
