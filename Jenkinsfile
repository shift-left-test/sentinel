pipeline {
    agent none
    stages {
        stage('Setup') {
            steps {
                updateGitlabCommitStatus name: 'build', state: 'pending'
            }
        }
        stage('clang-dev-9') {
            agent {
                docker { image 'cart.lge.com/swte/clang-dev:9' }
            }
            steps {
                cleanWs disableDeferredWipeout: true
                git branch: "${env.gitlabSourceBranch}", url: 'http://mod.lge.com/hub/yocto/addons/sentinel.git'
                withEnv(['GTEST_OUTPUT=xml:result/']) {
                    sh './build.sh'
                    sh 'gcovr --xml coverage.xml'
                }
            }
            post {
                success {
                    junit 'test/result/*.xml'
                    cobertura coberturaReportFile: 'coverage.xml'
                }
            }
        }
        stage('clang-dev-10') {
            agent {
                docker { image 'cart.lge.com/swte/clang-dev:10' }
            }
            steps {
                cleanWs disableDeferredWipeout: true
                git branch: "${env.gitlabSourceBranch}", url: 'http://mod.lge.com/hub/yocto/addons/sentinel.git'
                withEnv(['GTEST_OUTPUT=xml:result/']) {
                    sh './build.sh'
                    sh 'gcovr --xml coverage.xml'
                }
            }
            post {
                success {
                    junit 'test/result/*.xml'
                    cobertura coberturaReportFile: 'coverage.xml'
                }
            }
        }
        stage('clang-dev-11') {
            agent {
                docker { image 'cart.lge.com/swte/clang-dev:11' }
            }
            steps {
                cleanWs disableDeferredWipeout: true
                git branch: "${env.gitlabSourceBranch}", url: 'http://mod.lge.com/hub/yocto/addons/sentinel.git'
                withEnv(['GTEST_OUTPUT=xml:result/']) {
                    sh './build.sh'
                    sh 'gcovr --xml coverage.xml'
                }
            }
            post {
                success {
                    junit 'test/result/*.xml'
                    cobertura coberturaReportFile: 'coverage.xml'
                }
            }
        }
    }
    post {
        failure {
            updateGitlabCommitStatus name: 'build', state: 'failed'
        }
        success {
            updateGitlabCommitStatus name: 'build', state: 'success'
        }
    }
}
