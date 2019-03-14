7950 // Shell.
7951 
7952 #include "types.h"
7953 #include "user.h"
7954 #include "fcntl.h"
7955 
7956 // Parsed command representation
7957 #define EXEC  1
7958 #define REDIR 2
7959 #define PIPE  3
7960 #define LIST  4
7961 #define BACK  5
7962 
7963 #define MAXARGS 10
7964 
7965 struct cmd {
7966   int type;
7967 };
7968 
7969 struct execcmd {
7970   int type;
7971   char *argv[MAXARGS];
7972   char *eargv[MAXARGS];
7973 };
7974 
7975 struct redircmd {
7976   int type;
7977   struct cmd *cmd;
7978   char *file;
7979   char *efile;
7980   int mode;
7981   int fd;
7982 };
7983 
7984 struct pipecmd {
7985   int type;
7986   struct cmd *left;
7987   struct cmd *right;
7988 };
7989 
7990 struct listcmd {
7991   int type;
7992   struct cmd *left;
7993   struct cmd *right;
7994 };
7995 
7996 struct backcmd {
7997   int type;
7998   struct cmd *cmd;
7999 };
8000 int fork1(void);  // Fork but panics on failure.
8001 void panic(char*);
8002 struct cmd *parsecmd(char*);
8003 
8004 // Execute cmd.  Never returns.
8005 void
8006 runcmd(struct cmd *cmd)
8007 {
8008   int p[2];
8009   struct backcmd *bcmd;
8010   struct execcmd *ecmd;
8011   struct listcmd *lcmd;
8012   struct pipecmd *pcmd;
8013   struct redircmd *rcmd;
8014 
8015   if(cmd == 0)
8016     exit();
8017 
8018   switch(cmd->type){
8019   default:
8020     panic("runcmd");
8021 
8022   case EXEC:
8023     ecmd = (struct execcmd*)cmd;
8024     if(ecmd->argv[0] == 0)
8025       exit();
8026     exec(ecmd->argv[0], ecmd->argv);
8027     printf(2, "exec %s failed\n", ecmd->argv[0]);
8028     break;
8029 
8030   case REDIR:
8031     rcmd = (struct redircmd*)cmd;
8032     close(rcmd->fd);
8033     if(open(rcmd->file, rcmd->mode) < 0){
8034       printf(2, "open %s failed\n", rcmd->file);
8035       exit();
8036     }
8037     runcmd(rcmd->cmd);
8038     break;
8039 
8040   case LIST:
8041     lcmd = (struct listcmd*)cmd;
8042     if(fork1() == 0)
8043       runcmd(lcmd->left);
8044     wait();
8045     runcmd(lcmd->right);
8046     break;
8047 
8048 
8049 
8050   case PIPE:
8051     pcmd = (struct pipecmd*)cmd;
8052     if(pipe(p) < 0)
8053       panic("pipe");
8054     if(fork1() == 0){
8055       close(1);
8056       dup(p[1]);
8057       close(p[0]);
8058       close(p[1]);
8059       runcmd(pcmd->left);
8060     }
8061     if(fork1() == 0){
8062       close(0);
8063       dup(p[0]);
8064       close(p[0]);
8065       close(p[1]);
8066       runcmd(pcmd->right);
8067     }
8068     close(p[0]);
8069     close(p[1]);
8070     wait();
8071     wait();
8072     break;
8073 
8074   case BACK:
8075     bcmd = (struct backcmd*)cmd;
8076     if(fork1() == 0)
8077       runcmd(bcmd->cmd);
8078     break;
8079   }
8080   exit();
8081 }
8082 
8083 int
8084 getcmd(char *buf, int nbuf)
8085 {
8086   printf(2, "$ ");
8087   memset(buf, 0, nbuf);
8088   gets(buf, nbuf);
8089   if(buf[0] == 0) // EOF
8090     return -1;
8091   return 0;
8092 }
8093 
8094 
8095 
8096 
8097 
8098 
8099 
8100 int
8101 main(void)
8102 {
8103   static char buf[100];
8104   int fd;
8105 
8106   // Assumes three file descriptors open.
8107   while((fd = open("console", O_RDWR)) >= 0){
8108     if(fd >= 3){
8109       close(fd);
8110       break;
8111     }
8112   }
8113 
8114   // Read and run input commands.
8115   while(getcmd(buf, sizeof(buf)) >= 0){
8116     if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
8117       // Clumsy but will have to do for now.
8118       // Chdir has no effect on the parent if run in the child.
8119       buf[strlen(buf)-1] = 0;  // chop \n
8120       if(chdir(buf+3) < 0)
8121         printf(2, "cannot cd %s\n", buf+3);
8122       continue;
8123     }
8124     if(fork1() == 0)
8125       runcmd(parsecmd(buf));
8126     wait();
8127   }
8128   exit();
8129 }
8130 
8131 void
8132 panic(char *s)
8133 {
8134   printf(2, "%s\n", s);
8135   exit();
8136 }
8137 
8138 int
8139 fork1(void)
8140 {
8141   int pid;
8142 
8143   pid = fork();
8144   if(pid == -1)
8145     panic("fork");
8146   return pid;
8147 }
8148 
8149 
8150 // Constructors
8151 
8152 struct cmd*
8153 execcmd(void)
8154 {
8155   struct execcmd *cmd;
8156 
8157   cmd = malloc(sizeof(*cmd));
8158   memset(cmd, 0, sizeof(*cmd));
8159   cmd->type = EXEC;
8160   return (struct cmd*)cmd;
8161 }
8162 
8163 struct cmd*
8164 redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
8165 {
8166   struct redircmd *cmd;
8167 
8168   cmd = malloc(sizeof(*cmd));
8169   memset(cmd, 0, sizeof(*cmd));
8170   cmd->type = REDIR;
8171   cmd->cmd = subcmd;
8172   cmd->file = file;
8173   cmd->efile = efile;
8174   cmd->mode = mode;
8175   cmd->fd = fd;
8176   return (struct cmd*)cmd;
8177 }
8178 
8179 struct cmd*
8180 pipecmd(struct cmd *left, struct cmd *right)
8181 {
8182   struct pipecmd *cmd;
8183 
8184   cmd = malloc(sizeof(*cmd));
8185   memset(cmd, 0, sizeof(*cmd));
8186   cmd->type = PIPE;
8187   cmd->left = left;
8188   cmd->right = right;
8189   return (struct cmd*)cmd;
8190 }
8191 
8192 
8193 
8194 
8195 
8196 
8197 
8198 
8199 
8200 struct cmd*
8201 listcmd(struct cmd *left, struct cmd *right)
8202 {
8203   struct listcmd *cmd;
8204 
8205   cmd = malloc(sizeof(*cmd));
8206   memset(cmd, 0, sizeof(*cmd));
8207   cmd->type = LIST;
8208   cmd->left = left;
8209   cmd->right = right;
8210   return (struct cmd*)cmd;
8211 }
8212 
8213 struct cmd*
8214 backcmd(struct cmd *subcmd)
8215 {
8216   struct backcmd *cmd;
8217 
8218   cmd = malloc(sizeof(*cmd));
8219   memset(cmd, 0, sizeof(*cmd));
8220   cmd->type = BACK;
8221   cmd->cmd = subcmd;
8222   return (struct cmd*)cmd;
8223 }
8224 
8225 
8226 
8227 
8228 
8229 
8230 
8231 
8232 
8233 
8234 
8235 
8236 
8237 
8238 
8239 
8240 
8241 
8242 
8243 
8244 
8245 
8246 
8247 
8248 
8249 
8250 // Parsing
8251 
8252 char whitespace[] = " \t\r\n\v";
8253 char symbols[] = "<|>&;()";
8254 
8255 int
8256 gettoken(char **ps, char *es, char **q, char **eq)
8257 {
8258   char *s;
8259   int ret;
8260 
8261   s = *ps;
8262   while(s < es && strchr(whitespace, *s))
8263     s++;
8264   if(q)
8265     *q = s;
8266   ret = *s;
8267   switch(*s){
8268   case 0:
8269     break;
8270   case '|':
8271   case '(':
8272   case ')':
8273   case ';':
8274   case '&':
8275   case '<':
8276     s++;
8277     break;
8278   case '>':
8279     s++;
8280     if(*s == '>'){
8281       ret = '+';
8282       s++;
8283     }
8284     break;
8285   default:
8286     ret = 'a';
8287     while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
8288       s++;
8289     break;
8290   }
8291   if(eq)
8292     *eq = s;
8293 
8294   while(s < es && strchr(whitespace, *s))
8295     s++;
8296   *ps = s;
8297   return ret;
8298 }
8299 
8300 int
8301 peek(char **ps, char *es, char *toks)
8302 {
8303   char *s;
8304 
8305   s = *ps;
8306   while(s < es && strchr(whitespace, *s))
8307     s++;
8308   *ps = s;
8309   return *s && strchr(toks, *s);
8310 }
8311 
8312 struct cmd *parseline(char**, char*);
8313 struct cmd *parsepipe(char**, char*);
8314 struct cmd *parseexec(char**, char*);
8315 struct cmd *nulterminate(struct cmd*);
8316 
8317 struct cmd*
8318 parsecmd(char *s)
8319 {
8320   char *es;
8321   struct cmd *cmd;
8322 
8323   es = s + strlen(s);
8324   cmd = parseline(&s, es);
8325   peek(&s, es, "");
8326   if(s != es){
8327     printf(2, "leftovers: %s\n", s);
8328     panic("syntax");
8329   }
8330   nulterminate(cmd);
8331   return cmd;
8332 }
8333 
8334 struct cmd*
8335 parseline(char **ps, char *es)
8336 {
8337   struct cmd *cmd;
8338 
8339   cmd = parsepipe(ps, es);
8340   while(peek(ps, es, "&")){
8341     gettoken(ps, es, 0, 0);
8342     cmd = backcmd(cmd);
8343   }
8344   if(peek(ps, es, ";")){
8345     gettoken(ps, es, 0, 0);
8346     cmd = listcmd(cmd, parseline(ps, es));
8347   }
8348   return cmd;
8349 }
8350 struct cmd*
8351 parsepipe(char **ps, char *es)
8352 {
8353   struct cmd *cmd;
8354 
8355   cmd = parseexec(ps, es);
8356   if(peek(ps, es, "|")){
8357     gettoken(ps, es, 0, 0);
8358     cmd = pipecmd(cmd, parsepipe(ps, es));
8359   }
8360   return cmd;
8361 }
8362 
8363 struct cmd*
8364 parseredirs(struct cmd *cmd, char **ps, char *es)
8365 {
8366   int tok;
8367   char *q, *eq;
8368 
8369   while(peek(ps, es, "<>")){
8370     tok = gettoken(ps, es, 0, 0);
8371     if(gettoken(ps, es, &q, &eq) != 'a')
8372       panic("missing file for redirection");
8373     switch(tok){
8374     case '<':
8375       cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
8376       break;
8377     case '>':
8378       cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
8379       break;
8380     case '+':  // >>
8381       cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
8382       break;
8383     }
8384   }
8385   return cmd;
8386 }
8387 
8388 
8389 
8390 
8391 
8392 
8393 
8394 
8395 
8396 
8397 
8398 
8399 
8400 struct cmd*
8401 parseblock(char **ps, char *es)
8402 {
8403   struct cmd *cmd;
8404 
8405   if(!peek(ps, es, "("))
8406     panic("parseblock");
8407   gettoken(ps, es, 0, 0);
8408   cmd = parseline(ps, es);
8409   if(!peek(ps, es, ")"))
8410     panic("syntax - missing )");
8411   gettoken(ps, es, 0, 0);
8412   cmd = parseredirs(cmd, ps, es);
8413   return cmd;
8414 }
8415 
8416 struct cmd*
8417 parseexec(char **ps, char *es)
8418 {
8419   char *q, *eq;
8420   int tok, argc;
8421   struct execcmd *cmd;
8422   struct cmd *ret;
8423 
8424   if(peek(ps, es, "("))
8425     return parseblock(ps, es);
8426 
8427   ret = execcmd();
8428   cmd = (struct execcmd*)ret;
8429 
8430   argc = 0;
8431   ret = parseredirs(ret, ps, es);
8432   while(!peek(ps, es, "|)&;")){
8433     if((tok=gettoken(ps, es, &q, &eq)) == 0)
8434       break;
8435     if(tok != 'a')
8436       panic("syntax");
8437     cmd->argv[argc] = q;
8438     cmd->eargv[argc] = eq;
8439     argc++;
8440     if(argc >= MAXARGS)
8441       panic("too many args");
8442     ret = parseredirs(ret, ps, es);
8443   }
8444   cmd->argv[argc] = 0;
8445   cmd->eargv[argc] = 0;
8446   return ret;
8447 }
8448 
8449 
8450 // NUL-terminate all the counted strings.
8451 struct cmd*
8452 nulterminate(struct cmd *cmd)
8453 {
8454   int i;
8455   struct backcmd *bcmd;
8456   struct execcmd *ecmd;
8457   struct listcmd *lcmd;
8458   struct pipecmd *pcmd;
8459   struct redircmd *rcmd;
8460 
8461   if(cmd == 0)
8462     return 0;
8463 
8464   switch(cmd->type){
8465   case EXEC:
8466     ecmd = (struct execcmd*)cmd;
8467     for(i=0; ecmd->argv[i]; i++)
8468       *ecmd->eargv[i] = 0;
8469     break;
8470 
8471   case REDIR:
8472     rcmd = (struct redircmd*)cmd;
8473     nulterminate(rcmd->cmd);
8474     *rcmd->efile = 0;
8475     break;
8476 
8477   case PIPE:
8478     pcmd = (struct pipecmd*)cmd;
8479     nulterminate(pcmd->left);
8480     nulterminate(pcmd->right);
8481     break;
8482 
8483   case LIST:
8484     lcmd = (struct listcmd*)cmd;
8485     nulterminate(lcmd->left);
8486     nulterminate(lcmd->right);
8487     break;
8488 
8489   case BACK:
8490     bcmd = (struct backcmd*)cmd;
8491     nulterminate(bcmd->cmd);
8492     break;
8493   }
8494   return cmd;
8495 }
8496 
8497 
8498 
8499 
