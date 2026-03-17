pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = "TinaluxValidationApp"
include(":app")
include(":tinalux-sdk")

project(":tinalux-sdk").projectDir = file("../tinalux-sdk")
