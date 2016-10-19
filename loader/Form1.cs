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
            //string cmd = string.Empty;
            //string url = @"http://10.19.96.77:8080/session/" + Environment.UserName;
            //HttpWebRequest request = (HttpWebRequest)WebRequest.Create(url);
            //request.AutomaticDecompression = DecompressionMethods.GZip;

            //using (HttpWebResponse response = (HttpWebResponse)request.GetResponse())
            //using (Stream stream = response.GetResponseStream())
            //using (StreamReader reader = new StreamReader(stream))
            //{
            //    cmd = reader.ReadToEnd();
            //}
            //Process.Start("C:\\pulsar-windows\\conn.bat");
            string path = @"\\cwstorage1\tmp$\" + Environment.UserName;
            string cmd = File.ReadAllText(path);

            string token_path = @"\\cwstorage1\tmp$\" + Environment.UserName + @"_token";
            string token = File.ReadAllText(token_path);
            Process.Start("C:\\pulsar-windows\\pulsar.exe", "\"" + cmd + "\" ws://10.10.219.132:9000/?type=server&token=" + token);
            Environment.Exit(1);
        }
    }
}
