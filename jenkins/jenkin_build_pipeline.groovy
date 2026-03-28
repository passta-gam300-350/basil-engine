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
                context    : params.STATUS_CONTEXT ?: 'jenkins/build',
                description: description,
                target_url : env.BUILD_URL
            ])

            powershell """
                \$token = \$env:GITHUB_STATUS_TOKEN.Trim()
                \$headers = @{
                    Authorization = "Bearer \$token"
                    Accept = 'application/vnd.github+json'
                    'User-Agent' = 'jenkins-build-check'
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

pipeline {
    agent none

    parameters {
        string(name: 'GIT_SHA', defaultValue: '', description: 'Git commit SHA from GitHub.')
        string(name: 'GIT_REF', defaultValue: '', description: 'Git ref or branch name from GitHub.')
        string(name: 'PR_NUMBER', defaultValue: '', description: 'Pull request number.')
        string(name: 'REPO_FULL_NAME', defaultValue: '', description: 'GitHub owner/repository name.')
        string(name: 'PR_URL', defaultValue: '', description: 'Pull request URL.')
        string(name: 'STATUS_CONTEXT', defaultValue: 'jenkins/build', description: 'GitHub commit status context.')
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
                echo "PR #${params.PR_NUMBER} from ${params.REPO_FULL_NAME}"
                echo "Ref: ${params.GIT_REF}"
                echo "SHA: ${params.GIT_SHA}"
                echo "Status context: ${params.STATUS_CONTEXT}"
            }
        }

        stage('Build') {
            agent {
                label 'Windows Agent 01'
            }

            stages {
                stage('Checkout') {
                    steps {
                        script {
                            def remoteConfig = scm.userRemoteConfigs[0]
                            def branchName = params.GIT_REF?.trim() ? params.GIT_REF.trim() : 'main'

                            echo "SCM class: ${scm.getClass().name}"
                            echo "SCM branch target: ${branchName}"
                            echo "SCM remote URL: ${remoteConfig.url}"
                            echo "SCM credentials ID: ${remoteConfig.credentialsId}"

                            deleteDir()
                            checkout([
                                $class: 'GitSCM',
                                branches: [[name: "*/${branchName}"]],
                                doGenerateSubmoduleConfigurations: false,
                                extensions: [[
                                    $class: 'CloneOption',
                                    depth: 1,
                                    honorRefspec: true,
                                    noTags: true,
                                    shallow: true,
                                    timeout: 60
                                ]],
                                userRemoteConfigs: [[
                                    credentialsId: remoteConfig.credentialsId,
                                    refspec: "+refs/heads/${branchName}:refs/remotes/origin/${branchName}",
                                    url: remoteConfig.url
                                ]]
                            ])
                        }
                    }
                }

                stage('Install Mono Dependency') {
                    steps {
                        powershell '''
                            if (-not (Test-Path "dep/vendor/mono")) {
                                git clone https://github.com/Silver1713/MONO.git dep/vendor/mono
                            }
                        '''
                    }
                }

                stage('Configure CMake') {
                    steps {
                        powershell 'cmake --% -S . -B build -A x64 -DCMAKE_POLICY_VERSION_MINIMUM=3.5'
                    }
                }

                stage('Build Release') {
                    steps {
                        powershell 'cmake --build build --config Release --parallel'
                    }
                }

                stage('Build Debug') {
                    steps {
                        powershell 'cmake --build build --config Debug --parallel'
                    }
                }
            }

        }
    }

    post {
        success {
            script {
                updateGitHubStatus('success', 'Jenkins build succeeded')
            }
        }
        failure {
            script {
                updateGitHubStatus('failure', 'Jenkins build failed')
            }
        }
        aborted {
            script {
                updateGitHubStatus('error', 'Jenkins build aborted')
            }
        }
        unstable {
            script {
                updateGitHubStatus('failure', 'Jenkins build unstable')
            }
        }
    }
}
