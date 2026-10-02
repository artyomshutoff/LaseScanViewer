param([string]$Output="$PSScriptRoot/../build/web_assets.hpp")
$ErrorActionPreference='Stop'
$project=Split-Path $PSScriptRoot -Parent
$assetText=[System.Text.StringBuilder]::new()
[void]$assetText.AppendLine('#pragma once')
[void]$assetText.AppendLine('#include <map>')
[void]$assetText.AppendLine('const std::map<std::string,std::pair<std::string,std::string>> webAssets={')
foreach($entry in @(@('/index.html','text/html; charset=utf-8','web/index.html'),@('/style.css','text/css; charset=utf-8','web/style.css'),@('/manual.js','text/javascript; charset=utf-8','web/manual.js'),@('/viewer.js','text/javascript; charset=utf-8','web/viewer.js'),@('/logo.svg','image/svg+xml','logo2.svg'))){
 $content=[System.IO.File]::ReadAllText((Join-Path $project $entry[2]))
 if($content.Contains(')LSV_WEB_ASSET"')){throw 'Raw string delimiter collision'}
 [void]$assetText.AppendLine('{"'+$entry[0]+'",{"'+$entry[1]+'",R"LSV_WEB_ASSET('+ $content +')LSV_WEB_ASSET"}},')
}
[void]$assetText.AppendLine('};')
[System.IO.Directory]::CreateDirectory((Split-Path $Output -Parent)) | Out-Null
[System.IO.File]::WriteAllText($Output,$assetText.ToString(),[System.Text.UTF8Encoding]::new($false))
