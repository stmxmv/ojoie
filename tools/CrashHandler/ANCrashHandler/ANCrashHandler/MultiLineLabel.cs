using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ANCrashHandler
{
    public class MultiLineLabel : TextBox
    {

        private const int WM_SETFOCUS = 0x07;
        private const int WM_ENABLE = 0x0A;
        private const int WM_SETCURSOR = 0x20;
        private const int WM_LBUTTONDOWN = 0x0201;

        protected override void WndProc(ref System.Windows.Forms.Message m)
        {
            if (!(m.Msg == WM_SETFOCUS || m.Msg == WM_ENABLE || m.Msg == WM_SETCURSOR || m.Msg == WM_LBUTTONDOWN))
                base.WndProc(ref m);
        }


        public MultiLineLabel()
        {
            Enabled = true;
            ReadOnly = true;
            ShortcutsEnabled = false;
            TabStop = false;
            Multiline = true;
            BorderStyle = BorderStyle.None;
        }
    }
}
