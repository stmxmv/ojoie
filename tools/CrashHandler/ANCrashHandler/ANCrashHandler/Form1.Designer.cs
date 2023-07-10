namespace ANCrashHandler
{
    partial class Form1
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
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
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            OKButton = new Button();
            CrashMessageTextBox = new TextBox();
            TopLabel = new Label();
            multiLineLabel1 = new MultiLineLabel();
            SuspendLayout();
            // 
            // OKButton
            // 
            OKButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            OKButton.Location = new Point(647, 512);
            OKButton.Name = "OKButton";
            OKButton.Size = new Size(123, 29);
            OKButton.TabIndex = 0;
            OKButton.Text = "Send Report";
            OKButton.UseVisualStyleBackColor = true;
            OKButton.Click += OKButton_Click;
            // 
            // CrashMessageTextBox
            // 
            CrashMessageTextBox.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            CrashMessageTextBox.Location = new Point(12, 148);
            CrashMessageTextBox.Multiline = true;
            CrashMessageTextBox.Name = "CrashMessageTextBox";
            CrashMessageTextBox.ReadOnly = true;
            CrashMessageTextBox.ScrollBars = ScrollBars.Vertical;
            CrashMessageTextBox.Size = new Size(758, 358);
            CrashMessageTextBox.TabIndex = 1;
            // 
            // TopLabel
            // 
            TopLabel.AutoSize = true;
            TopLabel.Font = new Font("Segoe UI", 12F, FontStyle.Bold, GraphicsUnit.Point);
            TopLabel.Location = new Point(12, 9);
            TopLabel.Name = "TopLabel";
            TopLabel.Size = new Size(292, 28);
            TopLabel.TabIndex = 2;
            TopLabel.Text = "An ojoie process has crashed!";
            // 
            // multiLineLabel1
            // 
            multiLineLabel1.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            multiLineLabel1.BorderStyle = BorderStyle.None;
            multiLineLabel1.Location = new Point(12, 55);
            multiLineLabel1.Multiline = true;
            multiLineLabel1.Name = "multiLineLabel1";
            multiLineLabel1.ReadOnly = true;
            multiLineLabel1.ShortcutsEnabled = false;
            multiLineLabel1.Size = new Size(758, 87);
            multiLineLabel1.TabIndex = 3;
            multiLineLabel1.TabStop = false;
            multiLineLabel1.Text = resources.GetString("multiLineLabel1.Text");
            // 
            // Form1
            // 
            AutoScaleDimensions = new SizeF(8F, 20F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(782, 553);
            Controls.Add(multiLineLabel1);
            Controls.Add(TopLabel);
            Controls.Add(CrashMessageTextBox);
            Controls.Add(OKButton);
            Icon = (Icon)resources.GetObject("$this.Icon");
            MinimumSize = new Size(800, 600);
            Name = "Form1";
            Text = "ANCrashHandler";
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion

        private Button OKButton;
        private TextBox CrashMessageTextBox;
        private Label TopLabel;
        private MultiLineLabel multiLineLabel1;
    }
}