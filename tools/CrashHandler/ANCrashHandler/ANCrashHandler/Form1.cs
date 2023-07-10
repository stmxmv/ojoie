using BinaryMapper.Windows.Minidump;

namespace ANCrashHandler
{
    public partial class Form1 : Form
    {
        public static readonly Dictionary<uint, string> ExceptionMap = new Dictionary<uint, string>
        {
            { 0x40010005, "a Control-C" },
            { 0x40010008, "a Control-Break" },
            { 0x80000002, "a Datatype Misalignment" },
            { 0x80000003, "a Breakpoint" },
            { 0xc0000005, "an Access Violation" },
            { 0xc0000006, "an In Page Error" },
            { 0xc0000017, "a No Memory" },
            { 0xc000001d, "an Illegal Instruction" },
            { 0xc0000025, "a Non-continuable Exception" },
            { 0xc0000026, "an Invalid Disposition" },
            { 0xc000008c, "a Array Bounds Exceeded" },
            { 0xc000008d, "a Float Denormal Operand" },
            { 0xc000008e, "a Float Divide by Zero" },
            { 0xc000008f, "a Float Inexact Result" },
            { 0xc0000090, "a Float Invalid Operation" },
            { 0xc0000091, "a Float Overflow" },
            { 0xc0000092, "a Float Stack Check" },
            { 0xc0000093, "a Float Underflow" },
            { 0xc0000094, "an Integer Divide by Zero" },
            { 0xc0000095, "an Integer Overflow" },
            { 0xc0000096, "a Privileged Instruction" },
            { 0xc00000fD, "a Stack Overflow" },
            { 0xc0000142, "a DLL Initialization Failed" },
            { 0xe06d7363, "a Microsoft C++ Exception" },
        };

        public static string GetExceptionDescription(uint exceptionCode)
        {
            if (ExceptionMap.TryGetValue(exceptionCode, out var desc))
            {
                return desc;
            }
            return "an Unknown exception type";
        }


        public Form1()
        {
            InitializeComponent();

            this.StartPosition = FormStartPosition.CenterScreen;
            CrashMessageTextBox.Text = "This is ANCrashHandler App, this app is only used when a crash occured";

            string[] args = Environment.GetCommandLineArgs();

            if (args.Length < 3)
            {
                return;
            }

            string appName = args[1];
            string dumpFilePath = args[2];

            TopLabel.Text += $" {appName}";
            
            var stream = File.OpenRead(dumpFilePath);

            if (!stream.CanRead)
            {
                return;
            }

            var minidumpMapper = new MinidumpMapper();
            var minidump = minidumpMapper.ReadMinidump(stream);

            var exceptionRecord = minidump.ExceptionStream.ExceptionRecord;

            string report = "";


            report += $"Process ID: {minidump.MiscInfoStream.ProcessId}\r\n";

            report += $"Exception Code: {exceptionRecord.ExceptionCode}\r\n";
            report +=
                $"Exception Description: {GetExceptionDescription((uint)exceptionRecord.ExceptionCode)}\r\n";

            report += $"Exception Address: 0x{exceptionRecord.ExceptionAddress:X}\r\n";

            // report system info
            report += $"Processor Architecture: {minidump.SystemInfoStream.ProcessorArchitecture}\r\n";
            report += $"Processor Level: {minidump.SystemInfoStream.ProcessorLevel}\r\n";
            report += $"Processor Revision: {minidump.SystemInfoStream.ProcessorRevision}\r\n";
            report += $"Number of Processors: {minidump.SystemInfoStream.NumberOfProcessors}\r\n";
            report += $"Product Type: {minidump.SystemInfoStream.ProductType}\r\n";
            report += $"Major Version: {minidump.SystemInfoStream.MajorVersion}\r\n";
            report += $"Minor Version: {minidump.SystemInfoStream.MinorVersion}\r\n";
            report += $"Build Number: {minidump.SystemInfoStream.BuildNumber}\r\n";
            report += $"Platform ID: {minidump.SystemInfoStream.PlatformId}\r\n";
            report += $"Suite Mask: {minidump.SystemInfoStream.SuiteMask}\r\n";

            string moduleList = "Modules: \r\n";
            foreach (var module in minidump.Modules)
            {
                moduleList += module.Key + "\r\n";
            }

            report += moduleList;

            CrashMessageTextBox.Text = report;
        }

        private void OKButton_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void SubLabelTextBox_Enter(object sender, EventArgs e)
        {
            //CrashMessageTextBox.Focus();
        }

        private void SubLabelTextBox_EnabledChanged(object sender, EventArgs e)
        {
            ((TextBox)sender).ForeColor = Color.Black;
        }
    }
}