using System.Diagnostics;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.CSharp.Syntax;
using Microsoft.CodeAnalysis.Text;

namespace AN;

public class MonoCompiler
{

    private static readonly IEnumerable<string> DefaultNamespaces =
        new[]
        {
            "System",
            "System.Collections.Generic",
            "System.IO",
            "System.Linq",
            "System.Threading",
            "System.Threading.Tasks"
        };
    
    private string _systemLibraryPath;

    private readonly IEnumerable<MetadataReference> _defaultReferences;
    
    
    public MonoCompiler(String systemLibraryPath)
    {
        _systemLibraryPath = systemLibraryPath;
        _defaultReferences =
            new[]
            {
                MetadataReference.CreateFromFile($"{_systemLibraryPath}/mscorlib.dll"),
                MetadataReference.CreateFromFile($"{_systemLibraryPath}/System.dll"),
                MetadataReference.CreateFromFile($"{_systemLibraryPath}/System.Core.dll"),
                MetadataReference.CreateFromFile($"{_systemLibraryPath}/System.Net.Http.dll"),
            };
    }
    
    
    public bool Compile(String assemblyName, IEnumerable<string> inputFilePaths, String outputAssemblyPath, String outputPdbPath)
    {

        List<SourceText> sourceTexts = new List<SourceText>();
        
        SourceText globalUsings = SourceText.From(
            @"global using global::System;
                 global using global::System.Collections.Generic;
                 global using global::System.IO;
                 global using global::System.Linq;
                 global using global::System.Net.Http;
                 global using global::System.Threading;
                 global using global::System.Threading.Tasks;");
        
        sourceTexts.Add(globalUsings);
        
        foreach (string inputFilePath in inputFilePaths)
        {
            using var fs = new FileStream(inputFilePath, FileMode.Open);
            {
                SourceText st = SourceText.From(fs);
                sourceTexts.Add(st);
            }
        }

        
        CSharpParseOptions option = new CSharpParseOptions(LanguageVersion.CSharp10, preprocessorSymbols: new List<string>() { "Debug"});


        List<SyntaxTree> syntaxTrees = new List<SyntaxTree>();
        foreach (var sourceText in sourceTexts)
        {
            SyntaxTree tree = CSharpSyntaxTree.ParseText(sourceText, option);
            // Debug.Assert(tree.GetDiagnostics().ToList().Count == 0);
            syntaxTrees.Add(tree);
        }
        
        
        // get nodes annotated as MethodImplAttribute
        // var methodImplAttributes = tree.GetRoot().DescendantNodes().OfType<AttributeSyntax>()
            // .Where(x => x.Name.ToString().Equals("MethodImplAttribute"));
        
        var compileOption = new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary)
            .WithOverflowChecks(true)
            .WithOptimizationLevel(OptimizationLevel.Release);
            // .WithUsings(DefaultNamespaces); /// this is not working
        
            
        var compilation = CSharpCompilation.Create(
            assemblyName, 
            syntaxTrees, 
            _defaultReferences,
            compileOption);
        
        
        var result = compilation.Emit(outputAssemblyPath, outputPdbPath);

        if (!result.Success)
        {
            foreach (Diagnostic diagnostic in result.Diagnostics)
            {
                Console.WriteLine(diagnostic.GetMessage());
            }
        }

        return result.Success;
    }
}