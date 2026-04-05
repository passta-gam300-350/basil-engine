def updateGitHubStatus(String state, String description) {
    if (!params.GIT_SHA?.trim() || !params.REPO_FULL_NAME?.trim()) {
        echo 'Skipping GitHub status update because SHA or repository name is missing.'
        return
    }

    def repoParts = params.REPO_FULL_NAME.tokenize('/')
    if (repoParts.size() != 2) {
        echo "Skipping GitHub status update because REPO_FULL_NAME is invalid: ${params.REPO_FULL_NAME}"
        return
    }

    node('Windows Agent 01') {
        withCredentials([string(credentialsId: 'github-status-token', variable: 'GITHUB_STATUS_TOKEN')]) {
            def payload = groovy.json.JsonOutput.toJson([
                state      : state,
                context    : params.STATUS_CONTEXT ?: 'jenkins/deploy',
                description: description,
                target_url : env.BUILD_URL
            ])

            powershell """
                \$token = \$env:GITHUB_STATUS_TOKEN.Trim()
                \$headers = @{
                    Authorization = "Bearer \$token"
                    Accept = 'application/vnd.github+json'
                    'User-Agent' = 'jenkins-deploy'
                }
                \$body = @'
${payload}
'@
                Invoke-RestMethod `
                    -Method Post `
                    -Uri 'https://api.github.com/repos/${repoParts[0]}/${repoParts[1]}/statuses/${params.GIT_SHA}' `
                    -Headers \$headers `
                    -Body \$body `
                    -ContentType 'application/json'
            """
        }
    }
}

boolean isReleaseTagRef(String ref) {
    def normalized = ref?.trim()
    return normalized?.startsWith('refs/tags/') && normalized.toLowerCase().contains('release')
}

String extractTagName(String ref) {
    def normalized = ref?.trim() ?: ''
    return normalized.replaceFirst('^refs/tags/', '')
}

pipeline {
    agent none

    parameters {
        string(name: 'GIT_SHA', defaultValue: '', description: 'Git commit SHA from GitHub.')
        string(name: 'GIT_REF', defaultValue: '', description: 'Git ref or branch name from GitHub.')
        string(name: 'PR_NUMBER', defaultValue: '', description: 'Pull request number.')
        string(name: 'REPO_FULL_NAME', defaultValue: '', description: 'GitHub owner/repository name.')
        string(name: 'PR_URL', defaultValue: '', description: 'Pull request URL.')
        string(name: 'STATUS_CONTEXT', defaultValue: 'jenkins/deploy', description: 'GitHub commit status context.')
    }

    options {
        disableConcurrentBuilds(abortPrevious: true)
        skipDefaultCheckout()
        timestamps()
    }

    stages {
        stage('Build Context') {
            agent any
            steps {
                script {
                    echo "Repository: ${params.REPO_FULL_NAME}"
                    echo "Ref: ${params.GIT_REF}"
                    echo "SHA: ${params.GIT_SHA}"
                    echo "Status context: ${params.STATUS_CONTEXT}"
                    echo "Release-tag deploy: ${isReleaseTagRef(params.GIT_REF)}"
                }
            }
        }

        stage('Release Deploy') {
            when {
                expression {
                    return isReleaseTagRef(params.GIT_REF)
                }
            }

            agent {
                label 'Windows Agent 01'
            }

            environment {
                GAME_REPO_URL = 'https://github.com/Silver1713/unity_levelsv2'
                GAME_REPO_DIR = '.jenkins\\unity_levelsv2'
                DIST_DIR = 'dist'
                RELEASE_BUILD_DIR = 'dist\\release_build'
                DEBUG_BUILD_DIR = 'dist\\debug_build'
                RELEASE_PACKAGE_DIR = 'dist\\release_package'
                DEBUG_PACKAGE_DIR = 'dist\\debug_package'
            }

            stages {
                stage('Checkout') {
                    steps {
                        script {
                            def remoteConfig = scm.userRemoteConfigs[0]
                            def gitRef = params.GIT_REF?.trim()
                            def isTag = gitRef?.startsWith('refs/tags/')
                            def branchName = gitRef?.replaceFirst('^refs/heads/', '') ?: 'main'
                            def branchSpec = isTag ? gitRef : "*/${branchName}"
                            def refspec = isTag
                                ? "+${gitRef}:${gitRef}"
                                : "+refs/heads/${branchName}:refs/remotes/origin/${branchName}"

                            echo "SCM class: ${scm.getClass().name}"
                            echo "SCM remote URL: ${remoteConfig.url}"
                            echo "SCM credentials ID: ${remoteConfig.credentialsId}"
                            echo "Checkout target: ${branchSpec}"

                            deleteDir()
                            checkout([
                                $class: 'GitSCM',
                                branches: [[name: branchSpec]],
                                doGenerateSubmoduleConfigurations: false,
                                extensions: [[
                                    $class: 'CloneOption',
                                    depth: 1,
                                    honorRefspec: true,
                                    noTags: !isTag,
                                    shallow: true,
                                    timeout: 60
                                ]],
                                userRemoteConfigs: [[
                                    credentialsId: remoteConfig.credentialsId,
                                    refspec: refspec,
                                    url: remoteConfig.url
                                ]]
                            ])
                        }
                    }
                }

                stage('Install Tooling') {
                    steps {
                        bat '''
                            echo === DEBUG START ===

                            IF NOT EXIST dep\\vendor\\mono (
                                echo Cloning mono...
                                git clone https://github.com/Silver1713/MONO.git dep\\vendor\\mono
                                IF %ERRORLEVEL% NEQ 0 (
                                    echo Git clone failed!
                                    exit /b %ERRORLEVEL%
                                )
                            ) ELSE (
                                echo mono already exists
                            )

                            where gh >nul 2>nul
                            IF %ERRORLEVEL% NEQ 0 (
                                echo Installing GitHub CLI...

                                where winget >nul 2>nul
                                IF %ERRORLEVEL% EQU 0 (
                                    winget install --id GitHub.cli -e --accept-source-agreements --accept-package-agreements --silent
                                ) ELSE (
                                    where choco >nul 2>nul
                                    IF %ERRORLEVEL% EQU 0 (
                                        choco install gh -y --no-progress
                                    ) ELSE (
                                        echo No package manager found
                                        exit /b 1
                                    )
                                )
                            )

                            echo === DEBUG END ===
                            '''
                    }
                }

                stage('Fetch Game Project') {
                    steps {
                        withCredentials([string(
                            credentialsId: scm.userRemoteConfigs[0].credentialsId,
                            variable: 'GIT_TOKEN'
                        )]) {
                            powershell '''
                                $ErrorActionPreference = 'Stop'

                                if (Test-Path $env:GAME_REPO_DIR) {
                                    Remove-Item $env:GAME_REPO_DIR -Recurse -Force
                                }
                                $repoUrl = $env:GAME_REPO_URL -replace "https://", "https://x-access-token:$env:GIT_TOKEN@"
                                git clone --depth 1 $repoUrl $env:GAME_REPO_DIR

                                $gitDir = Join-Path $env:GAME_REPO_DIR ".git"
                                if (Test-Path $gitDir) {
                                    Remove-Item $gitDir -Recurse -Force
                                }
                            '''
                        }
                    }
                }

                stage('Configure CMake') {
                    steps {
                        powershell 'cmake --% -S . -B build -A x64 -DCMAKE_POLICY_VERSION_MINIMUM=3.5'
                    }
                }

                stage('Build Release Targets') {
                    steps {
                        powershell 'cmake --build build --target application engine editor --config Release --parallel'
                    }
                }

                stage('Build Debug Targets') {
                    steps {
                        powershell 'cmake --build build --target application engine editor --config Debug --parallel'
                    }
                }

                stage('Assemble Dist') {
                    steps {
                        powershell '''
                            $ErrorActionPreference = 'Stop'

                            $workspace = (Resolve-Path ".").Path
                            $distDir = Join-Path $workspace $env:DIST_DIR
                            $releaseBuildDir = Join-Path $workspace $env:RELEASE_BUILD_DIR
                            $debugBuildDir = Join-Path $workspace $env:DEBUG_BUILD_DIR
                            $releasePackageDir = Join-Path $workspace $env:RELEASE_PACKAGE_DIR
                            $debugPackageDir = Join-Path $workspace $env:DEBUG_PACKAGE_DIR
                            $gameProjectDir = Join-Path $workspace $env:GAME_REPO_DIR

                            function Resolve-EditorExe([string]$config) {
                                $workspaceRoot = (Resolve-Path ".").Path
                                $candidates = @(
                                    (Join-Path $workspaceRoot ("bin\\" + $config + "\\editor.exe")),
                                    (Join-Path $workspaceRoot ("build\\bin\\" + $config + "\\editor.exe")),
                                    (Join-Path $workspaceRoot ("build\\editor\\" + $config + "\\editor.exe"))
                                )

                                foreach ($candidate in $candidates) {
                                    if (Test-Path $candidate) {
                                        return (Resolve-Path $candidate).Path
                                    }
                                }

                                $searchRoots = @(
                                    (Join-Path $workspaceRoot "bin"),
                                    (Join-Path $workspaceRoot "build")
                                ) | Where-Object { Test-Path $_ }

                                $match = Get-ChildItem -Path $searchRoots -Recurse -Filter editor.exe -ErrorAction SilentlyContinue |
                                    Where-Object { $_.FullName -match ([regex]::Escape("\\" + $config + "\\")) } |
                                    Select-Object -First 1

                                if ($match) {
                                    return $match.FullName
                                }

                                throw "Unable to locate editor.exe for config '$config'."
                            }

                            function Resolve-PackageRoot([string]$outputDir) {
                                if (-not (Test-Path $outputDir)) {
                                    throw "Build output directory not found: $outputDir"
                                }

                                $subdirs = Get-ChildItem -Path $outputDir -Directory -ErrorAction SilentlyContinue
                                if ($subdirs.Count -eq 1) {
                                    return $subdirs[0].FullName
                                }

                                return $outputDir
                            }

                            function Resolve-PackagedExe([string]$packageRoot) {
                                $exe = Get-ChildItem -Path $packageRoot -Recurse -Filter *.exe -ErrorAction SilentlyContinue |
                                    Where-Object {
                                        $_.Name -notin @('editor.exe', 'application.exe')
                                    } |
                                    Sort-Object FullName |
                                    Select-Object -First 1

                                if (-not $exe) {
                                    throw "Unable to locate packaged game executable under $packageRoot"
                                }

                                return $exe.FullName
                            }

                            function Prepare-ReleasePackage([string]$sourceRoot, [string]$targetRoot, [string]$gameRoot) {
                                if (Test-Path $targetRoot) {
                                    Remove-Item $targetRoot -Recurse -Force
                                }

                                New-Item -ItemType Directory -Path $targetRoot | Out-Null
                                Copy-Item (Join-Path $sourceRoot '*') $targetRoot -Recurse -Force

                                $mainExe = Resolve-PackagedExe $targetRoot
                                $renamedExe = Join-Path $targetRoot 'Tangled Memories.exe'
                                if ($mainExe -ne $renamedExe) {
                                    if (Test-Path $renamedExe) {
                                        Remove-Item $renamedExe -Force
                                    }
                                    Rename-Item $mainExe 'Tangled Memories.exe'
                                }

                                $iconPath = Join-Path $targetRoot 'Icon.ico'
                                $projectIcon = Join-Path $gameRoot 'Icon.ico'
                                if (-not (Test-Path $iconPath) -and (Test-Path $projectIcon)) {
                                    Copy-Item $projectIcon $iconPath -Force
                                }

                                return $renamedExe
                            }

                            foreach ($path in @($distDir, $releaseBuildDir, $debugBuildDir, $releasePackageDir, $debugPackageDir)) {
                                if (Test-Path $path) {
                                    Remove-Item $path -Recurse -Force
                                }
                            }

                            New-Item -ItemType Directory -Path $distDir | Out-Null

                            $releaseEditor = Resolve-EditorExe "Release"
                            $debugEditor = Resolve-EditorExe "Debug"

                            & $releaseEditor -b $gameProjectDir -o $releaseBuildDir
                            if ($LASTEXITCODE -ne 0) {
                                throw "Release editor CLI build failed with exit code $LASTEXITCODE"
                            }

                            & $debugEditor -b $gameProjectDir -o $debugBuildDir
                            if ($LASTEXITCODE -ne 0) {
                                throw "Debug editor CLI build failed with exit code $LASTEXITCODE"
                            }

                            $releasePackageRoot = Resolve-PackageRoot $releaseBuildDir
                            $debugPackageRoot = Resolve-PackageRoot $debugBuildDir

                            $releaseExe = Prepare-ReleasePackage $releasePackageRoot $releasePackageDir $gameProjectDir
                            $debugExe = Prepare-ReleasePackage $debugPackageRoot $debugPackageDir $gameProjectDir

                            Copy-Item $releaseExe (Join-Path $distDir 'release.exe') -Force
                            Copy-Item $debugExe (Join-Path $distDir 'debug.exe') -Force
                            Copy-Item 'installer\\InstallScript.iss' (Join-Path $distDir 'Inno.iss') -Force

                            $distGameDataDir = Join-Path $distDir 'game_data'
                            New-Item -ItemType Directory -Path $distGameDataDir | Out-Null
                            Copy-Item (Join-Path $releasePackageDir '*') $distGameDataDir -Recurse -Force
                        '''
                    }
                }

                stage('Build Installer') {
                    steps {
                        powershell '''
                            $ErrorActionPreference = 'Stop'

                            $workspace = (Resolve-Path ".").Path
                            $distDir = Join-Path $workspace $env:DIST_DIR
                            $releasePackageDir = Join-Path $workspace $env:RELEASE_PACKAGE_DIR
                            $installerRoot = Join-Path $workspace 'installer'
                            $installerGameDir = Join-Path $installerRoot 'GAMEDIRECTORY'
                            $installerOutputDir = Join-Path $installerRoot 'INSTALLER'
                            $redistDir = Join-Path $installerRoot 'REDIST'
                            $vcRedist = Join-Path $redistDir 'VC_redist.x64.exe'

                            function Resolve-IsccExe() {
                                $defaultPaths = @(
                                    'C:\\Program Files (x86)\\Inno Setup 6\\ISCC.exe',
                                    'C:\\Program Files\\Inno Setup 6\\ISCC.exe'
                                )

                                foreach ($path in $defaultPaths) {
                                    if (Test-Path $path) {
                                        return $path
                                    }
                                }

                                if (Get-Command choco -ErrorAction SilentlyContinue) {
                                    choco install innosetup -y --no-progress
                                }
                                elseif (Get-Command winget -ErrorAction SilentlyContinue) {
                                    winget install --id JRSoftware.InnoSetup -e --accept-source-agreements --accept-package-agreements --silent
                                }
                                else {
                                    throw "Inno Setup is not installed and no supported package manager was found."
                                }

                                foreach ($path in $defaultPaths) {
                                    if (Test-Path $path) {
                                        return $path
                                    }
                                }

                                throw "Unable to locate ISCC.exe after installation."
                            }

                            if (-not (Test-Path $releasePackageDir)) {
                                throw "Prepared release package directory not found: $releasePackageDir"
                            }

                            if (Test-Path $installerGameDir) {
                                Remove-Item $installerGameDir -Recurse -Force
                            }
                            New-Item -ItemType Directory -Path $installerGameDir | Out-Null
                            Copy-Item (Join-Path $releasePackageDir '*') $installerGameDir -Recurse -Force

                            if (-not (Test-Path $redistDir)) {
                                New-Item -ItemType Directory -Path $redistDir | Out-Null
                            }
                            if (-not (Test-Path $vcRedist)) {
                                Invoke-WebRequest -Uri 'https://aka.ms/vs/17/release/vc_redist.x64.exe' -OutFile $vcRedist
                            }

                            $iscc = Resolve-IsccExe
                            & $iscc (Join-Path $installerRoot 'InstallScript.iss')
                            if ($LASTEXITCODE -ne 0) {
                                throw "Inno Setup compilation failed with exit code $LASTEXITCODE"
                            }

                            $installerExe = Get-ChildItem -Path $installerOutputDir -Filter *.exe -ErrorAction SilentlyContinue |
                                Sort-Object LastWriteTime -Descending |
                                Select-Object -First 1

                            if ($installerExe) {
                                Copy-Item $installerExe.FullName $distDir -Force
                            }
                        '''
                    }
                }

                stage('Publish GitHub Release') {
                    steps {
                        script {
                            def repoParts = params.REPO_FULL_NAME.tokenize('/')
                            def tagName = extractTagName(params.GIT_REF)

                            if (repoParts.size() != 2) {
                                error "REPO_FULL_NAME is invalid: ${params.REPO_FULL_NAME}"
                            }

                            withCredentials([string(credentialsId: 'github-status-token', variable: 'GH_TOKEN')]) {
                                powershell """
                                    \$ErrorActionPreference = 'Stop'
                                    \$tag = '${tagName}'
                                    \$repo = '${repoParts[0]}/${repoParts[1]}'
                                    \$distDir = Join-Path (Resolve-Path '.').Path '${env.DIST_DIR}'
                                    \$gameDataDir = Join-Path \$distDir 'game_data'
                                    \$gameDataZip = Join-Path \$distDir 'game_data.zip'

                                    if (Test-Path \$gameDataZip) {
                                        Remove-Item \$gameDataZip -Force
                                    }
                                    if (Test-Path \$gameDataDir) {
                                        Compress-Archive -Path (Join-Path \$gameDataDir '*') -DestinationPath \$gameDataZip -Force
                                    }

                                    gh release view \$tag --repo \$repo *> \$null
                                    if (\$LASTEXITCODE -ne 0) {
                                        gh release create \$tag `
                                            --repo \$repo `
                                            --title \$tag `
                                            --notes "Automated Jenkins release for \$tag"
                                    }

                                    \$assets = @()
                                    foreach (\$name in @('release.exe', 'debug.exe', 'Inno.iss', 'game_data.zip')) {
                                        \$candidate = Join-Path \$distDir \$name
                                        if (Test-Path \$candidate) {
                                            \$assets += \$candidate
                                        }
                                    }

                                    \$installer = Get-ChildItem -Path \$distDir -Filter *.exe -ErrorAction SilentlyContinue |
                                        Where-Object { \$_.Name -notin @('release.exe', 'debug.exe') } |
                                        Sort-Object Name |
                                        Select-Object -First 1
                                    if (\$installer) {
                                        \$assets += \$installer.FullName
                                    }

                                    if (\$assets.Count -eq 0) {
                                        throw 'No dist assets were produced for GitHub release upload.'
                                    }

                                    gh release upload \$tag --repo \$repo --clobber \$assets
                                """
                            }
                        }
                    }
                }
            }
        }

        stage('Skip Non-Release Ref') {
            when {
                expression {
                    return !isReleaseTagRef(params.GIT_REF)
                }
            }
            agent any
            steps {
                echo "Skipping deploy because ${params.GIT_REF} is not a release tag."
            }
        }
    }

    post {
        success {
            script {
                updateGitHubStatus('success', 'Jenkins deploy succeeded')
            }
        }
        failure {
            script {
                updateGitHubStatus('failure', 'Jenkins deploy failed')
            }
        }
        aborted {
            script {
                updateGitHubStatus('error', 'Jenkins deploy aborted')
            }
        }
        unstable {
            script {
                updateGitHubStatus('failure', 'Jenkins deploy unstable')
            }
        }
    }
}
