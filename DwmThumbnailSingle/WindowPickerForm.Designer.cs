using System.Windows.Forms;

namespace DwmThumbnailSingle
{
    partial class WindowPickerForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.listViewWindows = new System.Windows.Forms.ListView();
            this.SuspendLayout();
            // 
            // listViewWindows
            // 
            this.listViewWindows.HideSelection = false;
            this.listViewWindows.Location = new System.Drawing.Point(13, 13);
            this.listViewWindows.Name = "listViewWindows";
            this.listViewWindows.Size = new System.Drawing.Size(775, 425);
            this.listViewWindows.TabIndex = 0;
            this.listViewWindows.UseCompatibleStateImageBehavior = false;
            this.listViewWindows.FullRowSelect = true;
            this.listViewWindows.Columns.Add("Window", -2);
            this.listViewWindows.Dock = DockStyle.Fill;
            this.listViewWindows.ItemActivate += listViewWindows_ItemActivate;
            this.listViewWindows.View = View.Details;
            // 
            // WindowPickerForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 450);
            this.Controls.Add(this.listViewWindows);
            this.Name = "WindowPickerForm";
            this.Text = "WindowPickerForm";
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ListView listViewWindows;
    }
}