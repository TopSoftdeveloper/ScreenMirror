using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Win32.DwmThumbnail.Interop;

namespace DwmThumbnailSingle
{
    public partial class Form1 : Form
    {
        private IntPtr _hThumbnail = IntPtr.Zero;
        private IntPtr _hWnd = IntPtr.Zero;
        public Form1()
        {
            InitializeComponent();
            Load += OnLoaded;
            SizeChanged += OnSizeChanged;
        }

        private void OnLoaded(object sender, EventArgs e)
        {
            do
            {
                _hWnd = new WindowPickerForm().PickCaptureTarget(this.Handle);
            } while (_hWnd == IntPtr.Zero);

            var hr = NativeMethods.DwmRegisterThumbnail(this.Handle, _hWnd, out _hThumbnail);
            if (hr != 0)
                return;

            UpdateThumbnailProperties();
        }

        private void OnSizeChanged(object sender, EventArgs e)
        {
            if (_hThumbnail == IntPtr.Zero)
                return;

            UpdateThumbnailProperties();
        }

        private void UpdateThumbnailProperties()
        {
            var dpi = GetDpiScaleFactor(); // returns PointF

            // Use ClientSize (equivalent to WPF content area)
            int width = (int)(this.ClientSize.Width * dpi.X);
            int height = (int)(this.ClientSize.Height * dpi.Y);

            var props = new DWM_THUMBNAIL_PROPERTIES
            {
                fVisible = true,
                dwFlags = (int)(
                    DWM_TNP.DWM_TNP_VISIBLE |
                    DWM_TNP.DWM_TNP_OPACITY |
                    DWM_TNP.DWM_TNP_RECTDESTINATION |
                    DWM_TNP.DWM_TNP_SOURCECLIENTAREAONLY
                ),
                opacity = 255,
                rcDestination = new RECT
                {
                    left = 0,
                    top = 0,
                    right = width,
                    bottom = height
                },
                fSourceClientAreaOnly = true
            };

            NativeMethods.DwmUpdateThumbnailProperties(_hThumbnail, ref props);
        }

        private PointF GetDpiScaleFactor()
        {
            return new PointF(
                this.DeviceDpi / 96f,
                this.DeviceDpi / 96f
            );
        }
    }
}
