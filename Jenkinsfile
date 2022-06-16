@Library('jenkins-shared-library') _

pipeline {
    agent none
    options {
        parallelsAlwaysFailFast()
    }
    stages {
        stage('sentinel') {
            parallel {
                stage('clang-dev-9') {
                    agent { docker { image "${getDockerImage('clang-dev:9')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-10') {
                    agent { docker { image "${getDockerImage('clang-dev:10')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-11') {
                    agent { docker { image "${getDockerImage('clang-dev:11')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-12') {
                    agent { docker { image "${getDockerImage('clang-dev:12')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-13') {
                    agent { docker { image "${getDockerImage('clang-dev:13')}" } }
                    steps { testSentinel() }
                }
                stage('clang-dev-14') {
                    agent { docker { image "${getDockerImage('clang-dev:14')}" } }
                    steps { testSentinel() }
                }
            }
        }
    }
}
