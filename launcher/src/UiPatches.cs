using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Security.Cryptography;
using System.Text;

namespace JlhLauncher {
    static class UiPatches {
        static readonly string[] Files={"font.lmp","frontend.lmp","pause.lmp","control-layouts.bin","pt-BR/font.lmp","pt-BR/gametext.lmp","pt-BR/chartext.lmp","pt-BR/stats.lmp"};
        static byte[] Hash(byte[] value){using(var hash=SHA256.Create())return hash.ComputeHash(value);}
        static byte[] Read(BinaryReader r,int count){var bytes=r.ReadBytes(count);if(bytes.Length!=count)throw new IOException("Truncated UI patch.");return bytes;}
        public static byte[] Decode(string patch,string dump){
            using(var stream=File.OpenRead(patch))using(var r=new BinaryReader(stream,Encoding.UTF8)){
                if(Encoding.ASCII.GetString(Read(r,4))!="JLD1")throw new IOException("Invalid UI patch format.");
                uint count=r.ReadUInt32();if(count<1||count>8)throw new IOException("Invalid patch source count.");
                using(var bases=new MemoryStream()){
                    for(uint i=0;i<count;i++){
                        int length=r.ReadUInt16();if(length<1||length>80)throw new IOException("Invalid patch source name.");
                        string name=Encoding.UTF8.GetString(Read(r,length));
                        if(name.IndexOfAny(new[]{'/', '\\', ':'})>=0 || name.Contains("..") || !name.EndsWith(".lmp",StringComparison.Ordinal))throw new IOException("Unsafe patch source.");
                        byte[] expected=Read(r,32);string source=Path.Combine(dump,"res",name);
                        if(new FileInfo(source).Length>32*1024*1024)throw new IOException("Patch source too large.");
                        byte[] raw=File.ReadAllBytes(source);if(!Hash(raw).SequenceEqual(expected))throw new IOException("Resource version mismatch: "+name+". Use the supported original dump.");
                        if(bases.Length+raw.Length>128*1024*1024)throw new IOException("Patch inputs too large.");bases.Write(raw,0,raw.Length);
                    }
                    int size=r.ReadInt32();if(size<1||size>32*1024*1024)throw new IOException("Invalid patch output size.");
                    byte[] outputHash=Read(r,32),original=bases.ToArray(),output=new byte[size];int cursor=0;
                    using(var deflate=new DeflateStream(stream,CompressionMode.Decompress))using(var commands=new BinaryReader(deflate)){
                        while(cursor<size){byte op=commands.ReadByte();
                            if(op==0){uint offset=commands.ReadUInt32(),length=commands.ReadUInt32();if(length==0||offset>(ulong)original.Length||length>(ulong)original.Length-offset||length>(ulong)(size-cursor))throw new IOException("Invalid patch copy.");Buffer.BlockCopy(original,(int)offset,output,cursor,(int)length);cursor+=(int)length;}
                            else if(op==1){uint length=commands.ReadUInt32();if(length==0||length>(ulong)(size-cursor))throw new IOException("Invalid patch literal.");byte[] literal=Read(commands,(int)length);Buffer.BlockCopy(literal,0,output,cursor,(int)length);cursor+=(int)length;}
                            else throw new IOException("Unknown patch operation.");
                        }
                        if(commands.BaseStream.ReadByte()!=-1)throw new IOException("Trailing patch commands.");
                    }
                    if(!Hash(output).SequenceEqual(outputHash))throw new IOException("UI output integrity check failed.");return output;
                }
            }
        }
        public static void VerifyForTest(string root,string temp){
            string isolated=Path.Combine(temp,"patch-install");Directory.CreateDirectory(Path.Combine(isolated,"patches","ui"));
            foreach(string name in Files){string destination=Path.Combine(isolated,"patches","ui",name+".jldp");Directory.CreateDirectory(Path.GetDirectoryName(destination));File.Copy(Path.Combine(root,"patches","ui",name+".jldp"),destination);}
            Install(isolated,Path.Combine(root,"host","assets"));
            foreach(string name in Files)if(!File.ReadAllBytes(Path.Combine(isolated,"host","ui",name)).SequenceEqual(File.ReadAllBytes(Path.Combine(root,"host","ui",name))))throw new IOException("UI reconstruction differs: "+name);
            Install(isolated,Path.Combine(root,"host","assets"));
            string badDump=Path.Combine(temp,"bad-dump");Directory.CreateDirectory(Path.Combine(badDump,"res"));File.WriteAllBytes(Path.Combine(badDump,"res","font.lmp"),new byte[16]);
            bool rejected=false;try{Install(isolated,badDump);}catch(IOException){rejected=true;}if(!rejected)throw new IOException("Mismatched resource accepted.");
        }
        public static void Install(string root,string dump){
            string patches=Path.Combine(root,"patches","ui");
            // Development tree can retain its already-built overlays.
            if(!Directory.Exists(patches))throw new IOException("UI patch package missing. Extract the complete release package.");
            // Decode and validate every input/output before replacing any installed file.
            byte[][] outputs=new byte[Files.Length][];
            for(int i=0;i<Files.Length;i++)outputs[i]=Decode(Path.Combine(patches,Files[i]+".jldp"),dump);
            for(int i=0;i<Files.Length;i++){
                string path=Path.Combine(root,"host","ui",Files[i]);
                if(File.Exists(path)&&Hash(File.ReadAllBytes(path)).SequenceEqual(Hash(outputs[i])))continue;
                Directory.CreateDirectory(Path.GetDirectoryName(path));string temp=path+"."+Guid.NewGuid().ToString("N")+".tmp";
                try{File.WriteAllBytes(temp,outputs[i]);if(File.Exists(path))File.Replace(temp,path,null);else File.Move(temp,path);}finally{if(File.Exists(temp))File.Delete(temp);}
            }
        }
    }
}
