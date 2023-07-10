using AN;


using CommandLine;


class Options
{
    [Option('i', "input", Required = true, HelpText = "Input file")]
    public IEnumerable<string> InputFiles { get; set; }
 
    [Option('n', "name", Required = true, HelpText = "Assembly Name")]
    public string OutputName { get; set; }
    
    [Option('o', "name", Required = true, HelpText = "Output Path")]
    public string OutputPath { get; set; }
}

class MonoCompilerApp
{

    public static void Main(string[] args)
    {
        Parser.Default.ParseArguments<Options>(args)
            .WithParsed<Options>(o =>
            {
                MonoCompiler compiler = new MonoCompiler(@"C:\Users\Aleudillonam\CLionProjects\ojoie\cmake-build-ninja\bin\Debug\Data\Mono\lib\mono\4.8-api");
                
                if (!compiler.Compile(o.OutputName, o.InputFiles,
                        $"{o.OutputPath}/{o.OutputName}.dll",
                        $"{o.OutputPath}/{o.OutputName}.pdb"))
                {
                    Console.WriteLine("compile fail");
                }
            })
            .WithNotParsed(errors =>
            {
                foreach (var error in errors)
                {
                    Console.WriteLine(error);
                }
            });
    }
}


