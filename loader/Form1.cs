using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Net;
using System.IO;
using System.Diagnostics;

namespace loader
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
            string path = @"\\cwstorage1\tmp$\" + Environment.UserName;
            string cmd = File.ReadAllText(path);

            string token_path = @"\\cwstorage1\tmp$\" + Environment.UserName + @"_token";
            string token = File.ReadAllText(token_path);

            string arch_path = @"\\cwstorage1\tmp$\" + Environment.UserName + @"_arch";
            string arch = File.ReadAllText(arch_path);
            if (arch == @"x86")
            {
                Process.Start("C:\\pulsar-windows\\x86\\pulsar.exe", "\"" + cmd + "\" ws://10.10.219.132:9000/?type=server&token=" + token);
            }
            else
            {
                Process.Start("C:\\pulsar-windows\\pulsar.exe", "\"" + cmd + "\" ws://10.10.219.132:9000/?type=server&token=" + token);
            }
            Environment.Exit(1);
        }
    }
}
