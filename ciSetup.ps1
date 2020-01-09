Set-ExecutionPolicy Bypass -Scope Process -Force; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'));
choco install --no-progress --yes vulkan-sdk
[System.Environment]::SetEnvironmentVariable('VULKAN_SDK','C:\VulkanSDK\1.1.126.0',[System.EnvironmentVariableTarget]::User)
