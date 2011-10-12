param 
(
	[Parameter(Mandatory=$true)]
	$ProjectDirectory,

	[Parameter(Mandatory=$true)]
	$ProtoFile
)

cd $ProjectDirectory
$relativeProtoPath = resolve-path $ProtoFile -Relative

protoc.exe -I..\src --cpp_out=.\ $relativeProtoPath
