#!/usr/bin/env pwsh

$BucketAtom = "https://github.com/baulk/bucket/commits/master.atom"
Write-Host -ForegroundColor Green "Fetch $BucketAtom"

try {
    $obj = Invoke-WebRequest -Uri $BucketAtom -Method Get
    $xmlobj = [xml]$obj.Content
    $id=$xmlobj.feed.entry.id
    $commitid=$id.Split("/")[1]
    Write-Host -ForegroundColor Green "Latest commit is $commitid"
}
catch {
    Write-Host -ForcegroundColor Red "error $_"
}