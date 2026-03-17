package com.tinalux.runtime

class TinaluxVulkanValidationActivity : TinaluxActivity() {
    override fun preferredBackend(): TinaluxBackend = TinaluxBackend.Vulkan
}
