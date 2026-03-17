package com.tinalux.runtime

open class TinaluxVulkanValidationActivity : TinaluxActivity() {
    override fun preferredBackend(): TinaluxBackend = TinaluxBackend.Vulkan
}
