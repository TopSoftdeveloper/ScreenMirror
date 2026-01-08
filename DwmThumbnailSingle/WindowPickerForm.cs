using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Win32.DwmThumbnail.Interop;

namespace DwmThumbnailSingle
{
    public partial class WindowPickerForm : Form
    {
        private readonly string[] _ignoreProcesses =
        {
            "applicationframehost",
            "shellexperiencehost",
            "systemsettings",
            "winstore.app",
            "searchui"
        };

        public WindowPickerForm()
        {
            InitializeComponent();
            Load += OnLoad;
        }

        private void OnLoad(object sender, EventArgs e)
        {
            FindWindows();
        }

        /// <summary>
        /// Equivalent of PickCaptureTarget
        /// </summary>
        public IntPtr PickCaptureTarget(IntPtr ownerHwnd)
        {
            var owner = new Win32Window(ownerHwnd);
            ShowDialog(owner);

            return listViewWindows.SelectedItems.Count > 0
                ? ((CapturableWindow)listViewWindows.SelectedItems[0].Tag).Handle
                : IntPtr.Zero;
        }

        private void FindWindows()
        {
            NativeMethods.EnumWindows((hWnd, lParam) =>
            {
                // ignore invisible windows
                if (!NativeMethods.IsWindowVisible(hWnd))
                    return true;

                // ignore untitled windows
                var title = new StringBuilder(1024);
                NativeMethods.GetWindowText(hWnd, title, title.Capacity);
                if (string.IsNullOrWhiteSpace(title.ToString()))
                    return true;

                // ignore myself
                if (Handle == hWnd)
                    return true;

                NativeMethods.GetWindowThreadProcessId(hWnd, out var processId);

                Process process;
                try
                {
                    process = Process.GetProcessById((int)processId);
                }
                catch
                {
                    return true;
                }

                if (_ignoreProcesses.Contains(process.ProcessName.ToLower()))
                    return true;

                var window = new CapturableWindow
                {
                    Handle = hWnd,
                    Name = $"{title} ({process.ProcessName}.exe)"
                };

                var item = new ListViewItem(window.Name)
                {
                    Tag = window
                };

                listViewWindows.Items.Add(item);
                return true;

            }, IntPtr.Zero);
        }

        private void listViewWindows_ItemActivate(object sender, EventArgs e)
        {
            Close();
        }
    }

    internal sealed class Win32Window : IWin32Window
    {
        public IntPtr Handle { get; }

        public Win32Window(IntPtr handle)
        {
            Handle = handle;
        }
    }

    public struct CapturableWindow
    {
        public string Name { get; set; }
        public IntPtr Handle { get; set; }
    }
}
