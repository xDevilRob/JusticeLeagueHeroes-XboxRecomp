using System;
using System.IO;
using System.Linq;
using System.Xml.Serialization;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Windows.Forms;

namespace JlhLauncher {
    public class Preferences { public string Dump = ""; public string Repository = "xDevilRob/JusticeLeagueHeroes-XboxRecomp"; }
    static class Storage {
        public static readonly int[,] Resolutions={{1280,720},{1920,1080},{2560,1440},{3840,2160}};
        public static string[] ResolutionNames={"1280 x 720  (720p)","1920 x 1080  (1080p)","2560 x 1440  (1440p)","3840 x 2160  (4K)"};
        public static int ReadResolution(string path) {
            string text=File.Exists(path)?File.ReadAllText(path):"";
            string root=System.Text.RegularExpressions.Regex.Split(text,@"(?m)^\s*\[")[0];
            var w=System.Text.RegularExpressions.Regex.Match(root,@"(?m)^\s*width\s*=\s*(\d+)");
            var h=System.Text.RegularExpressions.Regex.Match(root,@"(?m)^\s*height\s*=\s*(\d+)");
            int width,height;
            if(!Int32.TryParse(w.Groups[1].Value,out width) || !Int32.TryParse(h.Groups[1].Value,out height))return -1;
            for(int i=0;i<4;i++)if(width==Resolutions[i,0] && height==Resolutions[i,1])return i;
            return -1;
        }
        public static readonly string[] DisplayModes={"windowed","borderless","fullscreen"};
        public static readonly string[] DisplayModeNames={"Windowed","Borderless","Fullscreen"};
        public static int ReadDisplayMode(string path) {
            string text=File.Exists(path)?File.ReadAllText(path):"";
            string root=System.Text.RegularExpressions.Regex.Split(text,@"(?m)^\s*\[")[0];
            var match=System.Text.RegularExpressions.Regex.Match(root,@"(?m)^\s*display_mode\s*=\s*""([^""]*)""");
            return Array.IndexOf(DisplayModes,match.Groups[1].Value);
        }
        public static void SaveResolution(string path,int choice,string backupRoot,string mode=null) {
            if(choice<0 || choice>=4)throw new IOException("Choose one of the four supported resolutions.");
            if(mode!=null && Array.IndexOf(DisplayModes,mode)<0)throw new IOException("Choose a supported screen mode.");
            string text=File.Exists(path)?File.ReadAllText(path):"";
            var section=System.Text.RegularExpressions.Regex.Match(text,@"(?m)^\s*\[");
            int end=section.Success?section.Index:text.Length;
            string head=text.Substring(0,end),tail=text.Substring(end);
            string newline=text.Contains("\r\n")?"\r\n":"\n";
            string[] keys={"width","height"};
            for(int i=0;i<2;i++){
                var pattern=new System.Text.RegularExpressions.Regex(@"(?m)^(\s*"+keys[i]+@"\s*=\s*)\d+([^\r\n]*)");
                if(pattern.Matches(head).Count>1)throw new IOException("Duplicate resolution setting. Review settings.toml first.");
                int value=Resolutions[choice,i];
                if(pattern.IsMatch(head))head=pattern.Replace(head,m=>m.Groups[1].Value+value+m.Groups[2].Value);
                else head+=(head.Length>0&&!head.EndsWith("\n")?newline:"")+keys[i]+" = "+value+newline;
            }
            if(mode!=null){
                var pattern=new System.Text.RegularExpressions.Regex(@"(?m)^(\s*display_mode\s*=\s*)""[^""\r\n]*""([^\r\n]*)");
                if(pattern.Matches(head).Count>1)throw new IOException("Duplicate screen mode setting. Review settings.toml first.");
                if(pattern.IsMatch(head))head=pattern.Replace(head,m=>m.Groups[1].Value+"\""+mode+"\""+m.Groups[2].Value);
                else head+=(head.Length>0&&!head.EndsWith("\n")?newline:"")+"display_mode = \""+mode+"\""+newline;
            }
            string result=head+tail;
            Directory.CreateDirectory(Path.GetDirectoryName(path));Directory.CreateDirectory(backupRoot);
            string temp=path+"."+Guid.NewGuid().ToString("N")+".tmp";
            try{
                File.WriteAllText(temp,result,new UTF8Encoding(false));
                if(File.Exists(path))File.Replace(temp,path,Path.Combine(backupRoot,"settings-"+DateTime.Now.ToString("yyyyMMdd-HHmmss")+"-"+Guid.NewGuid().ToString("N")+".toml"));
                else File.Move(temp,path);
            }finally{if(File.Exists(temp))File.Delete(temp);}
        }
        public const string XbeSha1 = "c94d34f77bfaa0b879bd5d53e3c62fb6b7bdef98";
        public static string Hash(string path) { using(var s=File.OpenRead(path)) using(var h=SHA1.Create()) return BitConverter.ToString(h.ComputeHash(s)).Replace("-", "").ToLowerInvariant(); }
        public static void ValidateDump(string folder) {
            if(!Directory.Exists(folder)) throw new IOException("Select the folder containing default.xbe and res.");
            string xbe=Path.Combine(folder,"default.xbe");
            if(!File.Exists(xbe) || !Directory.Exists(Path.Combine(folder,"res"))) throw new IOException("This folder must contain default.xbe and the res folder. Extracted Xbox dumps only in this version.");
            if(Hash(xbe)!=XbeSha1) throw new IOException("This default.xbe does not match the Xbox version supported by this recompilation.");
            foreach(string name in new[]{"gamedata.lmp","frontend.lmp","superman.lmp","batman.lmp"})
                if(!File.Exists(Path.Combine(folder,"res",name))) throw new IOException("Missing game file: res/"+name);
        }
        public static string Inside(string root,string relative) {
            if(String.IsNullOrWhiteSpace(relative) || Path.IsPathRooted(relative) || relative.Contains(":")) throw new IOException("Invalid archive path.");
            string prefix=Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar)+Path.DirectorySeparatorChar;
            string result=Path.GetFullPath(Path.Combine(prefix,relative.Replace('/',Path.DirectorySeparatorChar)));
            if(!result.StartsWith(prefix,StringComparison.OrdinalIgnoreCase)) throw new IOException("Archive path leaves the save folder.");
            return result;
        }
        public static void NoLinks(string root) {
            if(!Directory.Exists(root)) return;
            if((File.GetAttributes(root)&FileAttributes.ReparsePoint)!=0) throw new IOException("Linked save folders are not supported.");
            foreach(string d in Directory.GetFileSystemEntries(root)) {
                if((File.GetAttributes(d)&FileAttributes.ReparsePoint)!=0) throw new IOException("Linked save folders are not supported.");
                if(Directory.Exists(d)) NoLinks(d);
            }
        }
        public static string Backup(string saveRoot,string backupRoot) {
            if(!Directory.Exists(saveRoot) || !Directory.GetFiles(saveRoot,"slot.bin",SearchOption.AllDirectories).Any()) throw new IOException("No game saves were found.");
            NoLinks(saveRoot); Directory.CreateDirectory(backupRoot);
            string name="JLH-saves-"+DateTime.Now.ToString("yyyyMMdd-HHmmss-fff")+"-"+Guid.NewGuid().ToString("N").Substring(0,4)+".zip";
            string dest=Path.Combine(backupRoot,name); string temp=dest+".partial";
            try { ZipFile.CreateFromDirectory(saveRoot,temp,CompressionLevel.Optimal,false); File.Move(temp,dest); }
            finally { if(File.Exists(temp)) File.Delete(temp); }
            return dest;
        }
        public static void Restore(string archive,string saveRoot,string backupRoot) {
            // Validate the complete archive before touching existing saves.
            using(var zip=ZipFile.OpenRead(archive)) {
                long total=0; var names=new HashSet<string>(StringComparer.OrdinalIgnoreCase); bool hasSave=false;
                if(zip.Entries.Count>10000) throw new IOException("Too many files in the backup.");
                foreach(var e in zip.Entries) {
                    string dest=Inside(saveRoot,e.FullName); total+=e.Length;
                    if(total>128L*1024*1024) throw new IOException("This backup exceeds the save size limit.");
                    if(e.FullName.EndsWith("/")) continue;
                    if(!names.Add(dest)) throw new IOException("Duplicate file in backup.");
                    string file=Path.GetFileName(dest).ToLowerInvariant();
                    if(!new[]{"slot.bin","savemeta.xbx","saveimage.xbx","titleimage.xbx","titlemeta.xbx"}.Contains(file)) throw new IOException("Unexpected save file: "+file);
                    if(file=="slot.bin") { if(e.Length!=53268) throw new IOException("Unexpected slot.bin format."); hasSave=true; }
                }
                if(!hasSave) throw new IOException("No Justice League Heroes save was found in this ZIP.");
                NoLinks(saveRoot);
                if(Directory.Exists(saveRoot) && Directory.GetFiles(saveRoot,"slot.bin",SearchOption.AllDirectories).Any()) Backup(saveRoot,backupRoot);
                string stage=Path.Combine(backupRoot,"restore-"+Guid.NewGuid().ToString("N")); Directory.CreateDirectory(stage);
                try {
                    foreach(var e in zip.Entries) {
                        if(e.FullName.EndsWith("/")) continue;
                        string dest=Inside(stage,e.FullName);Directory.CreateDirectory(Path.GetDirectoryName(dest));e.ExtractToFile(dest);
                    }
                    foreach(string file in Directory.GetFiles(stage,"*",SearchOption.AllDirectories)) {
                        string relative=file.Substring(stage.Length+1);string dest=Inside(saveRoot,relative);
                        Directory.CreateDirectory(Path.GetDirectoryName(dest));File.Copy(file,dest,true);
                    }
                } finally { Directory.Delete(stage,true); }
            }
        }
    }
    class HeroPanel : Panel {
        public HeroPanel(){DoubleBuffered=true;}
        protected override void OnPaintBackground(PaintEventArgs e) {
            using(var brush=new LinearGradientBrush(ClientRectangle,Color.FromArgb(18,52,84),Color.FromArgb(6,17,31),25)) e.Graphics.FillRectangle(brush,ClientRectangle);
            e.Graphics.SmoothingMode=SmoothingMode.AntiAlias;
            using(var pen=new Pen(Color.FromArgb(32,117,174),1)) for(int x=-500;x<Width+400;x+=65) e.Graphics.DrawLine(pen,x,Height,x+500,0);
            using(var pen=new Pen(Color.FromArgb(46,149,202),2)) e.Graphics.DrawEllipse(pen,Width-365,-80,420,420);
            using(var pen=new Pen(Color.FromArgb(37,102,148),1)) e.Graphics.DrawEllipse(pen,Width-385,-100,460,460);
            Point[] shield={new Point(Width-275,60),new Point(Width-145,60),new Point(Width-160,155),new Point(Width-210,195),new Point(Width-260,155)};
            using(var fill=new SolidBrush(Color.FromArgb(22,37,53))) e.Graphics.FillPolygon(fill,shield);
            using(var pen=new Pen(Color.FromArgb(185,146,68),2)) e.Graphics.DrawPolygon(pen,shield);
            using(var font=new Font("Segoe UI",36,FontStyle.Bold)) using(var b=new SolidBrush(Color.FromArgb(217,185,112))) e.Graphics.DrawString("JL",font,b,Width-255,84);
        }
    }
    class MainForm : Form {
        readonly string root,data,saveRoot,backupRoot; Preferences prefs; readonly Panel content=new Panel();
        readonly Label status=new Label(); readonly Button play=new Button(); readonly Dictionary<string,Button> nav=new Dictionary<string,Button>();
        readonly Color navy=Color.FromArgb(10,22,37),card=Color.FromArgb(18,36,55),gold=Color.FromArgb(220,181,98),muted=Color.FromArgb(161,180,200);
        readonly System.Windows.Forms.Timer timer=new System.Windows.Forms.Timer(); Process game;
        public MainForm(string projectRoot) {
            root=projectRoot;data=Path.Combine(root,"launcher","data");saveRoot=Path.Combine(root,"host","run","hdd","E","UDATA","57520001");backupRoot=Path.Combine(data,"backups");
            Directory.CreateDirectory(data);prefs=LoadPreferences();
            Text="Justice League Heroes | Launcher";ClientSize=new Size(1060,690);MinimumSize=Size;MaximumSize=Size;
            Icon=Icon.ExtractAssociatedIcon(Application.ExecutablePath);
            StartPosition=FormStartPosition.CenterScreen;BackColor=navy;ForeColor=Color.White;Font=new Font("Segoe UI",10);AutoScaleMode=AutoScaleMode.Dpi;
            var header=new Panel{Dock=DockStyle.Top,Height=88,BackColor=Color.FromArgb(8,18,31)};
            header.Controls.Add(LabelAt("JUSTICE LEAGUE",26,16,600,30,19,FontStyle.Bold,Color.White));
            header.Controls.Add(LabelAt("H E R O E S   /   P C   L A U N C H E R",28,51,700,20,9,FontStyle.Regular,gold));
            header.Controls.Add(LabelAt("LOCAL BUILD  •  PRE-RELEASE",800,34,235,25,9,FontStyle.Regular,muted));
            var sidebar=new Panel{Dock=DockStyle.Left,Width=185,BackColor=Color.FromArgb(11,25,42)};
            int y=30;foreach(string section in new[]{"Game","Saves","Mods","Updates","Settings"}){string s=section;var b=ButtonAt(section,16,y,152,44);b.Click+=(a,e)=>ShowPage(s);sidebar.Controls.Add(b);nav.Add(s,b);y+=55;}
            sidebar.Controls.Add(LabelAt("YOUR GAME.\nYOUR PROGRESS.",23,450,145,60,10,FontStyle.Bold,muted));
            var bottom=new Panel{Dock=DockStyle.Bottom,Height=82,BackColor=Color.FromArgb(7,17,29)};
            status.SetBounds(24,22,750,48);status.ForeColor=muted;status.Font=new Font("Segoe UI",9);bottom.Controls.Add(status);
            play.SetBounds(835,16,198,49);StyleButton(play);play.Text="PLAY";play.BackColor=gold;play.ForeColor=navy;play.Font=new Font("Segoe UI",15,FontStyle.Bold);play.Click+=(a,e)=>Guard(Play);bottom.Controls.Add(play);
            content.Dock=DockStyle.Fill;content.Padding=new Padding(25);Controls.Add(content);Controls.Add(sidebar);Controls.Add(bottom);Controls.Add(header);
            timer.Interval=1200;timer.Tick+=(a,e)=>RefreshStatus();timer.Start();ShowPage("Game");RefreshStatus();
        }
        Preferences LoadPreferences(){string p=Path.Combine(data,"preferences.xml");try{if(File.Exists(p))using(var f=File.OpenRead(p))return (Preferences)new XmlSerializer(typeof(Preferences)).Deserialize(f);}catch{}return new Preferences();}
        void SaveDumpPath(){if(String.IsNullOrWhiteSpace(prefs.Dump))return;string p=Path.Combine(root,"host","run","dump-path.txt");Directory.CreateDirectory(Path.GetDirectoryName(p));File.WriteAllText(p+".tmp",prefs.Dump,new UTF8Encoding(false));if(File.Exists(p))File.Replace(p+".tmp",p,null);else File.Move(p+".tmp",p);}
        void SavePreferences(){SaveDumpPath();string p=Path.Combine(data,"preferences.xml");using(var f=File.Create(p+".tmp"))new XmlSerializer(typeof(Preferences)).Serialize(f,prefs);File.Copy(p+".tmp",p,true);File.Delete(p+".tmp");}
        string Dump { get { return String.IsNullOrWhiteSpace(prefs.Dump)?Path.Combine(root,"host","assets"):prefs.Dump; } }
        bool Running(){foreach(var p in Process.GetProcessesByName("JusticeLeagueHeroes")){p.Dispose();return true;}return false;}
        void RefreshStatus(){bool running=Running();play.Enabled=!running;status.Text=running?"Game is running. Close it before restoring saves or changing settings.":"Xbox dump: "+Dump+"\nChoose resolution in Settings. Controls and language remain in the game's Options menu.";}
        void Guard(Action a){try{a();}catch(Exception ex){MessageBox.Show(this,ex.Message,"Launcher",MessageBoxButtons.OK,MessageBoxIcon.Information);}}
        void Idle(){if(Running())throw new IOException("Close the game before continuing.");}
        Label LabelAt(string text,int x,int y,int w,int h,float size,FontStyle style,Color color){return new Label{Text=text,Left=x,Top=y,Width=w,Height=h,Font=new Font("Segoe UI",size,style),ForeColor=color,BackColor=Color.Transparent};}
        void StyleButton(Button b){b.FlatStyle=FlatStyle.Flat;b.FlatAppearance.BorderColor=Color.FromArgb(48,76,104);b.BackColor=card;b.ForeColor=Color.White;b.Cursor=Cursors.Hand;}
        Button ButtonAt(string text,int x,int y,int w,int h){var b=new Button{Text=text,Left=x,Top=y,Width=w,Height=h};StyleButton(b);return b;}
        void AddButton(string text,int x,int y,int w,Action a){var b=ButtonAt(text,x,y,w,38);b.Click+=(s,e)=>Guard(a);content.Controls.Add(b);}
        void TextBlock(string title,string text){content.Controls.Add(LabelAt(title,26,22,780,42,23,FontStyle.Bold,Color.White));content.Controls.Add(LabelAt(text,28,76,775,64,10,FontStyle.Regular,muted));}
        public void ShowPage(string name){content.Controls.Clear();foreach(var item in nav)item.Value.BackColor=item.Key==name?Color.FromArgb(31,61,88):card;
            if(name=="Game")GamePage();else if(name=="Saves")SavePage();else if(name=="Mods")ModsPage();else if(name=="Updates")UpdatesPage();else SettingsPage();}
        void GamePage(){var hero=new HeroPanel{Left=25,Top=25,Width=822,Height=265};
            hero.Controls.Add(LabelAt("RETURN TO THE LEAGUE",25,28,480,24,10,FontStyle.Bold,gold));
            hero.Controls.Add(LabelAt("JUSTICE LEAGUE\nHEROES",23,67,540,115,32,FontStyle.Bold,Color.White));
            hero.Controls.Add(LabelAt("Your Xbox adventure. Ready for Windows.",26,214,530,28,11,FontStyle.Regular,muted));content.Controls.Add(hero);
            content.Controls.Add(LabelAt("READY FOR YOUR NEXT MISSION",28,313,730,30,14,FontStyle.Bold,Color.White));
            content.Controls.Add(LabelAt("720p  /  1080p  /  1440p  /  4K\nChoose resolution in launcher Settings or in the game's Options menu.",29,355,750,65,11,FontStyle.Regular,muted));
            AddButton("Select Xbox dump",29,437,200,SelectDump);AddButton("Manage saves",245,437,180,()=>ShowPage("Saves"));AddButton("Resolution settings",441,437,205,()=>ShowPage("Settings"));}
        void SelectDump(){Idle();using(var d=new FolderBrowserDialog{Description="Select the extracted Xbox game folder (default.xbe + res)",SelectedPath=Dump}){if(d.ShowDialog(this)!=DialogResult.OK)return;Storage.ValidateDump(d.SelectedPath);UiPatches.Install(root,d.SelectedPath);prefs.Dump=d.SelectedPath;SavePreferences();RefreshStatus();MessageBox.Show(this,"Dump validated and PC improvements prepared. Original game files remain unchanged.","Xbox dump");}}
        void Play(){Idle();Storage.ValidateDump(Dump);string exe=Path.Combine(root,"host","run","JusticeLeagueHeroes.exe");if(!File.Exists(exe))throw new IOException("The game executable is missing. Build the game first.");
            UiPatches.Install(root,Dump);SaveDumpPath();
            var start=new ProcessStartInfo(exe){UseShellExecute=false,WorkingDirectory=Path.GetDirectoryName(exe)};start.EnvironmentVariables["JLH_ASSET_DIR"]=Dump;
            foreach(string key in new[]{"JLH_PAD_COMMAND_FILE","JLH_TEST_INPUT","JLH_MAX_LEVEL","JLH_TRACE_AUDIO","JLH_TRACE_CONTROLS","XBE_VERBOSE","XBE_PERF"})start.EnvironmentVariables.Remove(key);
            game=Process.Start(start);RefreshStatus();}
        void SavePage(){TextBlock("Your saves","Back up your progress before trying a new build. Restoring creates a backup first and preserves unrelated save slots.");
            var list=new ListBox{Left=28,Top=148,Width=774,Height=193,BackColor=card,ForeColor=Color.White,BorderStyle=BorderStyle.FixedSingle};content.Controls.Add(list);
            if(Directory.Exists(saveRoot))foreach(string p in Directory.GetFiles(saveRoot,"slot.bin",SearchOption.AllDirectories)){byte[] b=new byte[8];using(var f=File.OpenRead(p))f.Read(b,0,8);string mission=Encoding.ASCII.GetString(b).TrimEnd('\0');list.Items.Add(Path.GetFileName(Path.GetDirectoryName(p))+"   •   "+mission+"   •   "+File.GetLastWriteTime(p).ToString("g"));}
            if(list.Items.Count==0)list.Items.Add("No saves found yet. Start a new game to create one.");
            AddButton("Create backup",28,363,180,()=>{Idle();string p=Storage.Backup(saveRoot,backupRoot);MessageBox.Show(this,"Backup created:\n"+p,"Saves");});
            AddButton("Restore / import ZIP",224,363,215,()=>{Idle();using(var d=new OpenFileDialog{Filter="JLH save backups (*.zip)|*.zip",InitialDirectory=backupRoot}){if(d.ShowDialog(this)!=DialogResult.OK)return;if(MessageBox.Show(this,"Restore this backup? Matching save files will be replaced. A backup of current saves will be created first.","Restore saves",MessageBoxButtons.YesNo,MessageBoxIcon.Question)!=DialogResult.Yes)return;Storage.Restore(d.FileName,saveRoot,backupRoot);ShowPage("Saves");MessageBox.Show(this,"Save backup restored.","Saves");}});
            AddButton("Open backups folder",455,363,210,()=>{Directory.CreateDirectory(backupRoot);Process.Start(backupRoot);});
            content.Controls.Add(LabelAt("To export a backup, copy its ZIP from the backups folder.\nOriginal game files and Windows settings are never included.",29,427,770,55,10,FontStyle.Regular,muted));}
        void ModsPage(){TextBlock("Mods","Local mod library. The mod loading system is not implemented in this first launcher build.");string dir=Path.Combine(root,"mods");
            var list=new ListBox{Left=28,Top=148,Width=774,Height=230,BackColor=card,ForeColor=Color.White};content.Controls.Add(list);
            if(Directory.Exists(dir))foreach(string p in Directory.GetDirectories(dir))list.Items.Add(Path.GetFileName(p)+"  —  not loaded");if(list.Items.Count==0)list.Items.Add("No local mod folders.");
            AddButton("Open mods folder",28,404,200,()=>{Directory.CreateDirectory(dir);Process.Start(dir);});content.Controls.Add(LabelAt("Folders are listed for organization only. Nothing is injected into the game.",29,461,760,35,10,FontStyle.Regular,muted));}
        void UpdatesPage(){TextBlock("Project updates","Updates will come from published GitHub Releases. The repository has not been configured yet; automatic installation is not enabled.");
            content.Controls.Add(LabelAt("GITHUB REPOSITORY  (owner/repository)",29,158,700,25,10,FontStyle.Bold,gold));var field=new TextBox{Left=29,Top=195,Width=650,Text=prefs.Repository};content.Controls.Add(field);
            AddButton("Save repository",29,250,190,()=>{string value=field.Text.Trim();if(value.Length>0 && !System.Text.RegularExpressions.Regex.IsMatch(value,@"^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$"))throw new IOException("Use owner/repository, without a URL.");prefs.Repository=value;SavePreferences();});
            AddButton("Open GitHub Releases",235,250,230,()=>{if(String.IsNullOrWhiteSpace(prefs.Repository))throw new IOException("Configure the repository first.");Process.Start("https://github.com/"+prefs.Repository+"/releases");});
            content.Controls.Add(LabelAt("Next step: version checks, package verification and recovery backups.\nThis prototype does not download or replace project files.",29,327,770,65,11,FontStyle.Regular,muted));}
        void SettingsPage(){TextBlock("Launcher settings","Apply resolution and screen mode before starting the game. Controls and language are available in the game's Options menu.");
            string settings=Path.Combine(root,"host","run","settings.toml");
            content.Controls.Add(LabelAt("GAME RESOLUTION",29,148,365,25,10,FontStyle.Bold,gold));
            content.Controls.Add(LabelAt("SCREEN MODE",414,148,365,25,10,FontStyle.Bold,gold));
            var resolution=new ComboBox{Left=29,Top=182,Width=365,DropDownStyle=ComboBoxStyle.DropDownList,ForeColor=Color.Black,BackColor=Color.White};resolution.Items.AddRange(Storage.ResolutionNames);resolution.SelectedIndex=Storage.ReadResolution(settings);content.Controls.Add(resolution);resolution.SelectedIndex=Storage.ReadResolution(settings);
            var mode=new ComboBox{Left=414,Top=182,Width=365,DropDownStyle=ComboBoxStyle.DropDownList,ForeColor=Color.Black,BackColor=Color.White};mode.Items.AddRange(Storage.DisplayModeNames);mode.SelectedIndex=Storage.ReadDisplayMode(settings);content.Controls.Add(mode);mode.SelectedIndex=Storage.ReadDisplayMode(settings);
            var feedback=LabelAt("Changes apply on the next game launch.",185,225,610,42,10,FontStyle.Regular,muted);content.Controls.Add(feedback);
            AddButton("Apply",29,220,140,()=>{Idle();if(mode.SelectedIndex<0)throw new IOException("Choose a screen mode.");Storage.SaveResolution(settings,resolution.SelectedIndex,Path.Combine(data,"settings-backups"),Storage.DisplayModes[mode.SelectedIndex]);feedback.Text="Resolution and screen mode saved. Applies on the next game launch.";feedback.ForeColor=gold;});
            content.Controls.Add(LabelAt("SELECTED XBOX DUMP",29,280,730,25,10,FontStyle.Bold,gold));
            var path=new TextBox{Left=29,Top=313,Width=774,ReadOnly=true,Text=Dump};content.Controls.Add(path);AddButton("Choose dump folder",29,355,205,()=>{SelectDump();path.Text=Dump;});
            AddButton("Validate dump",250,355,180,()=>{Storage.ValidateDump(Dump);MessageBox.Show(this,"Supported default.xbe and required resource files found.","Validation");});
            content.Controls.Add(LabelAt("SAVE LOCATION\n"+saveRoot,29,420,775,65,10,FontStyle.Regular,muted));}
        protected override void Dispose(bool disposing){if(disposing){timer.Dispose();if(game!=null)game.Dispose();}base.Dispose(disposing);}
    }
    static class Program {
        [STAThread] static void Main(string[] args){Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
            string root=AppDomain.CurrentDomain.BaseDirectory.TrimEnd(Path.DirectorySeparatorChar);
            if(String.Equals(Path.GetFileName(root),"launcher",StringComparison.OrdinalIgnoreCase))root=Directory.GetParent(root).FullName;
            if(args.Length>0 && args[0]=="--self-test"){SelfTest.Run(root);return;}
            if(args.Length==2 && args[0]=="--prepare-ui"){Storage.ValidateDump(args[1]);UiPatches.Install(root,args[1]);File.WriteAllText(Path.Combine(root,"host","run","dump-path.txt"),args[1],new UTF8Encoding(false));return;}
            using(var form=new MainForm(root)){
                if(args.Length>=2 && args[0]=="--render"){form.Show();if(args.Length==3)form.ShowPage(args[2]);Application.DoEvents();using(var bitmap=new Bitmap(form.Width,form.Height)){form.DrawToBitmap(bitmap,new Rectangle(0,0,form.Width,form.Height));bitmap.Save(args[1]);}return;}
                Application.Run(form);
            }
        }
    }
    static class SelfTest {
        public static void Run(string root){string temp=Path.Combine(root,"analysis","launcher","self-test-"+Guid.NewGuid().ToString("N"));string saves=Path.Combine(temp,"saves"),backups=Path.Combine(temp,"backups");Directory.CreateDirectory(Path.Combine(saves,"slot1"));
            try{byte[] original=new byte[53268];Encoding.ASCII.GetBytes("e6l1").CopyTo(original,0);string slot=Path.Combine(saves,"slot1","slot.bin");File.WriteAllBytes(slot,original);
                string zip=Storage.Backup(saves,backups);File.WriteAllBytes(slot,new byte[53268]);Storage.Restore(zip,saves,backups);if(!File.ReadAllBytes(slot).SequenceEqual(original))throw new Exception("Restore roundtrip failed");
                if(Directory.GetFiles(backups,"*.zip").Length<2)throw new Exception("Pre-restore backup missing");
                string bad=Path.Combine(temp,"bad.zip");using(var z=ZipFile.Open(bad,ZipArchiveMode.Create)){var e=z.CreateEntry("../slot.bin");using(var s=e.Open())s.Write(original,0,original.Length);}
                bool rejected=false;try{Storage.Restore(bad,saves,backups);}catch(IOException){rejected=true;}if(!rejected)throw new Exception("Unsafe ZIP accepted");
                string config=Path.Combine(temp,"settings.toml");string unrelated="display_mode = \"windowed\"\nfps_cap = 60\nvsync = true\nbutton_layout = 4\nlanguage = 5\n";string nested="[other]\nwidth = 99\nheight = 77\n";
                File.WriteAllText(config,"width = 1280 # keep comment\nheight = 720\n"+unrelated+nested);
                for(int i=0;i<4;i++){Storage.SaveResolution(config,i,backups);if(Storage.ReadResolution(config)!=i)throw new Exception("Resolution roundtrip failed");string text=File.ReadAllText(config);if(!text.Contains(unrelated)||!text.Contains(nested)||!text.Contains("# keep comment"))throw new Exception("Unrelated settings changed");}
                File.WriteAllText(config,unrelated+nested);Storage.SaveResolution(config,2,backups);if(Storage.ReadResolution(config)!=2 || !File.ReadAllText(config).Contains(nested))throw new Exception("Missing root resolution failed");
                foreach(string mode in Storage.DisplayModes){Storage.SaveResolution(config,3,backups,mode);if(Storage.DisplayModes[Storage.ReadDisplayMode(config)]!=mode || !File.ReadAllText(config).Contains(nested))throw new Exception("Screen mode roundtrip failed");}
                File.WriteAllText(config,"fps_cap = 60\n"+nested);Storage.SaveResolution(config,1,backups,"borderless");if(Storage.ReadDisplayMode(config)!=1 || !File.ReadAllText(config).Contains(nested))throw new Exception("Missing root screen mode failed");
                string before=File.ReadAllText(config);rejected=false;try{Storage.SaveResolution(config,1,backups,"invalid");}catch(IOException){rejected=true;}if(!rejected || File.ReadAllText(config)!=before)throw new Exception("Invalid screen mode accepted");
                UiPatches.VerifyForTest(root,temp);Storage.ValidateDump(Path.Combine(root,"host","assets"));File.WriteAllText(Path.Combine(root,"analysis","launcher","self-test.txt"),"PASS: save roundtrip; automatic backup before restore; ZIP traversal rejection; supported dump validation; all four resolutions; all three screen modes; invalid mode rejection; unrelated TOML preservation; root insertion before tables.\n");
            }finally{Directory.Delete(temp,true);}
        }
    }
}


