/*
init		= hello_init,			    0
.getattr	= hello_getattr,	종현	1
.readdir	= hello_readdir,	강현	2
.open		= hello_open,		종현	3
.read		= hello_read,		민석	4
.create		영리						5
.mkdir		강현						6
.write		민석						7
.unlink		영리						8
.rmdir		강현						9

select는 유저 단위
thread는 유저 한명당이 아닌 요청 한번당

fuse daemon 파일 , connection 파일 헤더파일 분리 , TCP Boundary read/write
header 파일 분리 

마운트시 서버의 어떤 디렉토리와 연동할지 옵션을 주든지 해서
*/