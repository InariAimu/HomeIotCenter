using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.Net.Sockets;
using System.Net;

using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace HomeIoTCenter.BigIoT
{
    public class Device
    {
        public String Device_ID;
        public String Api_Key;
        
        public Socket ClientSocket;

        public Device(String DEVICE_ID,String API_KEY)
        {
            this.Device_ID = DEVICE_ID;
            this.Api_Key = API_KEY;
        }

        public bool InitConnection()
        {
            ClientSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            try
            {
                ClientSocket.Connect("www.bigiot.net", 8181);
            }
            catch
            {
                ClientSocket.Close();
                return false;
            }
            if (ClientSocket.Connected == false) return false;
            return true;
        }

        public bool Login()
        {
            JObject jo = new JObject();
            jo.Add("M", "checkin");
            jo.Add("ID", Device_ID);
            jo.Add("K", Api_Key);
            jo.ToString();

            ClientSocket.Send(Encoding.UTF8.GetBytes(jo.ToString() + "\n"));
            
            return true;
        }
    }
}
